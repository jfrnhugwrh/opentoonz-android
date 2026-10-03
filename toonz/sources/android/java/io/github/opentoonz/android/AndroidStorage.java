package io.github.opentoonz.android;

import android.app.Activity;
import android.content.ActivityNotFoundException;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.content.res.Configuration;
import android.database.Cursor;
import android.graphics.Point;
import android.net.Uri;
import android.os.Build;
import android.os.Environment;
import android.os.Handler;
import android.os.Looper;
import android.os.StatFs;
import android.provider.DocumentsContract;
import android.provider.MediaStore;
import android.provider.OpenableColumns;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.WindowManager;
import android.widget.Toast;

import androidx.annotation.Nullable;
import androidx.documentfile.provider.DocumentFile;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicReference;

/**
 * Storage and platform services backing the OpenToonz native layer.
 *
 * <p>Android applications may only open paths inside their own directories:
 * everything else is reached through the Storage Access Framework, which hands
 * out document URIs rather than file paths. The native code, however, works
 * exclusively with paths - the file format loaders, the scene builder and the
 * file browser all assume a POSIX style tree. The bridge resolves the
 * difference by mapping the granted trees into a synthetic directory:
 *
 * <pre>
 *   &lt;internal&gt;/saf/&lt;treeId&gt;/&lt;path inside the granted tree&gt;
 * </pre>
 *
 * <p>The {@code saf} component is reserved and is never created on disk, so the
 * native side can recognise a mapped path and route the operation here.
 *
 * <p>All methods are static and are called from native threads; anything that
 * touches the UI is marshalled back onto the main thread.
 */
public final class AndroidStorage {

    private static final String TAG = "OpenToonzStorage";

    /** Preference file holding the granted trees. */
    private static final String PREFS = "opentoonz_saf";
    private static final String KEY_TREES = "trees";
    private static final String KEY_NEXT_ID = "nextId";

    /** Separator used to pack the tree description into one string. */
    private static final char FIELD_SEP = '\u001f';

    /** How long a picker is allowed to keep the native side waiting. */
    private static final long PICKER_TIMEOUT_SECONDS = 600;

    private static Context applicationContext;
    private static Activity activity;
    private static volatile boolean initialized = false;

    /** Result slot for the blocking picker calls. */
    private static final AtomicReference<Uri> pendingPickerResult = new AtomicReference<>(null);
    private static volatile CountDownLatch pendingPickerLatch = null;
    private static volatile int pendingPickerRequestCode = -1;

    private static final int REQUEST_OPEN_DOCUMENT = 0x0D01;
    private static final int REQUEST_CREATE_DOCUMENT = 0x0D02;
    private static final int REQUEST_OPEN_TREE = 0x0D03;

    private AndroidStorage() {
    }

    // =====================================================================
    //  Lifecycle
    // =====================================================================

    static void attach(Context context) {
        applicationContext = context.getApplicationContext();
        initialized = true;
    }

    static void attachActivity(Activity a) {
        activity = a;
        if (a != null) {
            attach(a);
        }
    }

    private static Context context() {
        if (applicationContext == null) {
            throw new IllegalStateException("AndroidStorage has not been attached to a context");
        }
        return applicationContext;
    }

    private static boolean requireInitialized() {
        if (!initialized || applicationContext == null) {
            Log.e(TAG, "AndroidStorage used before initialization");
            return false;
        }
        return true;
    }

    // =====================================================================
    //  Application specific storage
    // =====================================================================

    /** Internal, path accessible storage; no permission required. */
    public static String getInternalRoot() {
        if (!requireInitialized()) {
            return "";
        }
        return context().getFilesDir().getAbsolutePath();
    }

    public static String getCacheRoot() {
        if (!requireInitialized()) {
            return "";
        }
        return context().getCacheDir().getAbsolutePath();
    }

    /**
     * Unpacks the "stuff" tree bundled in the APK assets into the application
     * specific storage, once. This is the Android equivalent of the desktop
     * install step: the APK is not a browsable directory, so the stock
     * configuration, layouts, palettes and libraries have to be materialised
     * before OpenToonz can read them.
     *
     * @param target absolute path the tree has to be extracted to
     * @return true when the tree exists (already extracted or just written)
     */
    public static boolean ensureAssetsExtracted(String target) {
        if (!requireInitialized() || target == null || target.isEmpty()) {
            return false;
        }

        File root = new File(target);
        File stamp = new File(root, ".assets-version");
        String version = assetsVersion();

        try {
            if (root.isDirectory() && stamp.isFile()
                    && version.equals(readTextFile(stamp).trim())) {
                return true;
            }

            // Extract into a staging directory and rename it into place, so an
            // interrupted extraction never leaves a half populated tree behind.
            File staging = new File(target + ".incomplete");
            deleteRecursively(staging);
            if (!staging.mkdirs() && !staging.isDirectory()) {
                Log.e(TAG, "cannot create " + staging);
                return false;
            }

            copyAssetTree("stuff", staging);

            if (root.exists()) {
                // The user may already have projects: keep them.
                mergeDirectories(staging, root);
            } else if (!staging.renameTo(root)) {
                mergeDirectories(staging, root);
                deleteRecursively(staging);
            }

            // Directories the application writes into and that may be absent
            // from the packaged tree.
            new File(root, "projects/library").mkdirs();
            new File(root, "projects/fxs").mkdirs();
            new File(root, "cache").mkdirs();
            new File(root, "profiles").mkdirs();

            writeTextFile(stamp, version);
            return true;
        } catch (IOException e) {
            Log.e(TAG, "extracting the bundled stuff tree failed", e);
            return root.isDirectory();
        }
    }

    private static String assetsVersion() {
        try {
            PackageManager pm = context().getPackageManager();
            return String.valueOf(pm.getPackageInfo(context().getPackageName(), 0).versionCode);
        } catch (Exception e) {
            return "1";
        }
    }

    private static void copyAssetTree(String assetPath, File destination) throws IOException {
        String[] children = context().getAssets().list(assetPath);
        if (children == null || children.length == 0) {
            copyAssetFile(assetPath, destination);
            return;
        }

        if (!destination.isDirectory() && !destination.mkdirs()) {
            throw new IOException("cannot create " + destination);
        }
        for (String child : children) {
            copyAssetTree(assetPath + "/" + child, new File(destination, child));
        }
    }

    private static void copyAssetFile(String assetPath, File destination) throws IOException {
        File parent = destination.getParentFile();
        if (parent != null && !parent.isDirectory() && !parent.mkdirs()) {
            throw new IOException("cannot create " + parent);
        }
        try (InputStream in = context().getAssets().open(assetPath);
             OutputStream out = new FileOutputStream(destination)) {
            byte[] buffer = new byte[64 * 1024];
            int read;
            while ((read = in.read(buffer)) > 0) {
                out.write(buffer, 0, read);
            }
        }
    }

    private static void mergeDirectories(File source, File destination) throws IOException {
        if (!destination.isDirectory() && !destination.mkdirs()) {
            throw new IOException("cannot create " + destination);
        }
        File[] children = source.listFiles();
        if (children == null) {
            return;
        }
        for (File child : children) {
            File target = new File(destination, child.getName());
            if (child.isDirectory()) {
                mergeDirectories(child, target);
            } else if (!target.exists()) {
                if (!child.renameTo(target)) {
                    copyFile(child, target);
                    //noinspection ResultOfMethodCallIgnored
                    child.delete();
                }
            }
        }
    }

    // =====================================================================
    //  Storage Access Framework - granted trees
    // =====================================================================

    private static SharedPreferences prefs() {
        return context().getSharedPreferences(PREFS, Context.MODE_PRIVATE);
    }

    /**
     * Returns the granted trees as {@code "<id>\u001f<uri>\u001f<title>"}
     * strings, which is what the native layer parses.
     */
    public static String[] trees() {
        if (!requireInitialized()) {
            return new String[0];
        }
        String packed = prefs().getString(KEY_TREES, "");
        if (packed == null || packed.isEmpty()) {
            return new String[0];
        }

        List<String> out = new ArrayList<>();
        for (String entry : packed.split("\n")) {
            if (entry.isEmpty()) {
                continue;
            }
            String[] fields = entry.split("\u001e", -1);
            if (fields.length < 3) {
                continue;
            }
            out.add(fields[0] + FIELD_SEP + fields[1] + FIELD_SEP + fields[2]);
        }
        return out.toArray(new String[0]);
    }

    /** Registers a tree uri; returns its numeric id, or 0 on failure. */
    public static int addTree(String treeUri, String title) {
        if (!requireInitialized() || treeUri == null || treeUri.isEmpty()) {
            return 0;
        }

        Uri uri = Uri.parse(treeUri);
        try {
            context().getContentResolver().takePersistableUriPermission(
                    uri, Intent.FLAG_GRANT_READ_URI_PERMISSION
                            | Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
        } catch (SecurityException e) {
            Log.w(TAG, "the persisted permission for " + treeUri + " is not available", e);
        }

        SharedPreferences prefs = prefs();
        String packed = prefs.getString(KEY_TREES, "");
        if (packed == null) {
            packed = "";
        }

        // Re-use the id when the same tree is granted again.
        for (String entry : packed.split("\n")) {
            if (entry.isEmpty()) {
                continue;
            }
            String[] fields = entry.split("\u001e", -1);
            if (fields.length >= 2 && fields[1].equals(treeUri)) {
                return Integer.parseInt(fields[0]);
            }
        }

        int id = prefs.getInt(KEY_NEXT_ID, 1);
        String label = (title == null || title.isEmpty()) ? treeUri : title;
        String record = id + "\u001e" + treeUri + "\u001e" + label;
        String updated = packed.isEmpty() ? record : packed + "\n" + record;
        prefs.edit().putString(KEY_TREES, updated).putInt(KEY_NEXT_ID, id + 1).apply();
        return id;
    }

    public static void removeTree(int id) {
        if (!requireInitialized()) {
            return;
        }
        SharedPreferences prefs = prefs();
        String packed = prefs.getString(KEY_TREES, "");
        if (packed == null || packed.isEmpty()) {
            return;
        }

        StringBuilder kept = new StringBuilder();
        for (String entry : packed.split("\n")) {
            if (entry.isEmpty()) {
                continue;
            }
            String[] fields = entry.split("\u001e", -1);
            if (fields.length >= 1 && String.valueOf(id).equals(fields[0])) {
                try {
                    context().getContentResolver().releasePersistableUriPermission(
                            Uri.parse(fields[1]),
                            Intent.FLAG_GRANT_READ_URI_PERMISSION
                                    | Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
                } catch (Exception e) {
                    Log.w(TAG, "releasing the permission for tree " + id + " failed", e);
                }
                continue;
            }
            if (kept.length() > 0) {
                kept.append('\n');
            }
            kept.append(entry);
        }
        prefs.edit().putString(KEY_TREES, kept.toString()).apply();
    }

    // =====================================================================
    //  SAF path mapping
    // =====================================================================

    /** Resolved (treeId, relative path) pair of a mapped path. */
    private static final class Mapping {
        final int treeId;
        final String relative;

        Mapping(int treeId, String relative) {
            this.treeId = treeId;
            this.relative = relative;
        }
    }

    @Nullable
    private static Mapping mapPath(String mappedPath) {
        if (mappedPath == null || mappedPath.isEmpty()) {
            return null;
        }
        File safRoot = new File(getInternalRoot(), "saf");
        String rootPath = safRoot.getAbsolutePath();
        if (!mappedPath.startsWith(rootPath + "/") && !mappedPath.startsWith(rootPath + File.separator)) {
            return null;
        }

        String rest = mappedPath.substring(rootPath.length() + 1);
        int sep = rest.indexOf('/');
        if (sep < 0) {
            sep = rest.indexOf(File.separatorChar);
        }
        String idPart = (sep < 0) ? rest : rest.substring(0, sep);
        String relative = (sep < 0) ? "" : rest.substring(sep + 1);

        try {
            int id = Integer.parseInt(idPart);
            return new Mapping(id, relative);
        } catch (NumberFormatException e) {
            return null;
        }
    }

    @Nullable
    private static String treeUriForId(int id) {
        String packed = prefs().getString(KEY_TREES, "");
        if (packed == null) {
            return null;
        }
        for (String entry : packed.split("\n")) {
            if (entry.isEmpty()) {
                continue;
            }
            String[] fields = entry.split("\u001e", -1);
            if (fields.length >= 2 && String.valueOf(id).equals(fields[0])) {
                return fields[1];
            }
        }
        return null;
    }

    /** Resolves a mapped path to the document it denotes. */
    @Nullable
    private static DocumentFile documentFor(String mappedPath) {
        Mapping mapping = mapPath(mappedPath);
        if (mapping == null) {
            return null;
        }
        String treeUri = treeUriForId(mapping.treeId);
        if (treeUri == null) {
            return null;
        }
        DocumentFile root = DocumentFile.fromTreeUri(context(), Uri.parse(treeUri));
        if (root == null || !root.isDirectory()) {
            return null;
        }
        if (mapping.relative.isEmpty()) {
            return root;
        }

        DocumentFile current = root;
        for (String part : mapping.relative.split("/")) {
            if (part.isEmpty()) {
                continue;
            }
            DocumentFile child = findChild(current, part);
            if (child == null) {
                return null;
            }
            current = child;
        }
        return current;
    }

    @Nullable
    private static DocumentFile findChild(DocumentFile parent, String name) {
        if (parent == null) {
            return null;
        }
        for (DocumentFile child : parent.listFiles()) {
            if (name.equals(child.getName())) {
                return child;
            }
        }
        return null;
    }

    /**
     * Lists a mapped directory. Directories are reported with a trailing
     * separator so that the native side can tell them apart without a second
     * call per entry.
     */
    public static String[] listDirectory(String mappedPath) {
        if (!requireInitialized()) {
            return new String[0];
        }
        DocumentFile directory = documentFor(mappedPath);
        if (directory == null || !directory.isDirectory()) {
            return new String[0];
        }

        List<String> out = new ArrayList<>();
        for (DocumentFile child : directory.listFiles()) {
            String name = child.getName();
            if (name == null || name.isEmpty()) {
                continue;
            }
            out.add(child.isDirectory() ? name + "/" : name);
        }

        Collections.sort(out, new Comparator<String>() {
            @Override
            public int compare(String a, String b) {
                return a.compareToIgnoreCase(b);
            }
        });
        return out.toArray(new String[0]);
    }

    public static boolean exists(String mappedPath) {
        return requireInitialized() && documentFor(mappedPath) != null;
    }

    public static boolean isDirectory(String mappedPath) {
        if (!requireInitialized()) {
            return false;
        }
        DocumentFile file = documentFor(mappedPath);
        return file != null && file.isDirectory();
    }

    public static boolean createDirectory(String mappedPath) {
        if (!requireInitialized()) {
            return false;
        }
        Mapping mapping = mapPath(mappedPath);
        if (mapping == null || mapping.relative.isEmpty()) {
            return false;
        }

        DocumentFile parent = documentFor(parentOf(mappedPath));
        if (parent == null || !parent.isDirectory()) {
            return false;
        }
        String name = new File(mapping.relative).getName();
        return parent.findFile(name) != null || parent.createDirectory(name) != null;
    }

    public static boolean remove(String mappedPath, boolean recursive) {
        if (!requireInitialized()) {
            return false;
        }
        DocumentFile file = documentFor(mappedPath);
        if (file == null) {
            return false;
        }
        if (recursive && file.isDirectory()) {
            return deleteRecursively(file);
        }
        return file.delete();
    }

    private static boolean deleteRecursively(DocumentFile file) {
        if (file.isDirectory()) {
            for (DocumentFile child : file.listFiles()) {
                if (!deleteRecursively(child)) {
                    return false;
                }
            }
        }
        return file.delete();
    }

    public static boolean rename(String fromMappedPath, String toMappedPath) {
        if (!requireInitialized()) {
            return false;
        }
        DocumentFile source = documentFor(fromMappedPath);
        if (source == null) {
            return false;
        }
        DocumentFile destinationParent = documentFor(parentOf(toMappedPath));
        if (destinationParent == null || !destinationParent.isDirectory()) {
            return false;
        }

        Mapping mapping = mapPath(toMappedPath);
        if (mapping == null) {
            return false;
        }
        String name = new File(mapping.relative).getName();
        return source.renameTo(name);
    }

    @Nullable
    private static String parentOf(String mappedPath) {
        int sep = mappedPath.lastIndexOf('/');
        if (sep < 0) {
            return null;
        }
        return mappedPath.substring(0, sep);
    }

    /**
     * Copies a SAF document into the application cache and returns the local
     * path, so that the untouched OpenToonz readers - which need random access
     * and seek - can load it.
     */
    public static String materialize(String mappedPath) {
        if (!requireInitialized()) {
            return "";
        }
        DocumentFile file = documentFor(mappedPath);
        if (file == null || !file.isFile()) {
            return "";
        }

        File staging = new File(stagingDirectory(), file.getName() == null ? "document" : file.getName());
        try (InputStream in = context().getContentResolver().openInputStream(file.getUri());
             OutputStream out = new FileOutputStream(staging)) {
            if (in == null) {
                return "";
            }
            byte[] buffer = new byte[64 * 1024];
            int read;
            while ((read = in.read(buffer)) > 0) {
                out.write(buffer, 0, read);
            }
        } catch (IOException e) {
            Log.e(TAG, "materializing " + mappedPath + " failed", e);
            return "";
        }
        return staging.getAbsolutePath();
    }

    /** Writes a local file back into its SAF document. */
    public static boolean store(String mappedPath, String localPath) {
        if (!requireInitialized()) {
            return false;
        }
        DocumentFile file = documentFor(mappedPath);
        if (file == null) {
            // The document does not exist yet: create it next to its siblings.
            DocumentFile parent = documentFor(parentOf(mappedPath));
            if (parent == null || !parent.isDirectory()) {
                return false;
            }
            Mapping mapping = mapPath(mappedPath);
            if (mapping == null) {
                return false;
            }
            String name = new File(mapping.relative).getName();
            String mime = mimeTypeOf(name);
            file = parent.createFile(mime, name);
            if (file == null) {
                return false;
            }
        }

        try (InputStream in = new FileInputStream(localPath);
             OutputStream out = context().getContentResolver().openOutputStream(file.getUri(), "wt")) {
            if (out == null) {
                return false;
            }
            byte[] buffer = new byte[64 * 1024];
            int read;
            while ((read = in.read(buffer)) > 0) {
                out.write(buffer, 0, read);
            }
            return true;
        } catch (IOException e) {
            Log.e(TAG, "storing " + localPath + " into " + mappedPath + " failed", e);
            return false;
        }
    }

    /** Copies a local file or directory into a mapped SAF directory. */
    public static boolean copyInto(String localSource, String mappedDirectory) {
        if (!requireInitialized()) {
            return false;
        }
        DocumentFile directory = documentFor(mappedDirectory);
        if (directory == null || !directory.isDirectory()) {
            return false;
        }
        return copyIntoDocument(new File(localSource), directory);
    }

    private static boolean copyIntoDocument(File source, DocumentFile destination) {
        if (source.isDirectory()) {
            DocumentFile child = destination.findFile(source.getName());
            if (child == null || !child.isDirectory()) {
                child = destination.createDirectory(source.getName());
            }
            if (child == null) {
                return false;
            }
            File[] children = source.listFiles();
            if (children != null) {
                for (File entry : children) {
                    if (!copyIntoDocument(entry, child)) {
                        return false;
                    }
                }
            }
            return true;
        }

        String name = source.getName();
        DocumentFile target = destination.findFile(name);
        if (target == null) {
            target = destination.createFile(mimeTypeOf(name), name);
        }
        if (target == null) {
            return false;
        }

        try (InputStream in = new FileInputStream(source);
             OutputStream out = context().getContentResolver().openOutputStream(target.getUri(), "wt")) {
            if (out == null) {
                return false;
            }
            byte[] buffer = new byte[64 * 1024];
            int read;
            while ((read = in.read(buffer)) > 0) {
                out.write(buffer, 0, read);
            }
            return true;
        } catch (IOException e) {
            Log.e(TAG, "copying " + source + " failed", e);
            return false;
        }
    }

    /**
     * Publishes a local file to the shared media collections through the
     * MediaStore, which is the supported way of writing to shared storage
     * without any permission on Android 10 and later.
     */
    public static boolean exportToMediaStore(String localPath, String mimeType) {
        if (!requireInitialized()) {
            return false;
        }
        File source = new File(localPath);
        if (!source.isFile()) {
            return false;
        }

        String mime = (mimeType == null || mimeType.isEmpty())
                ? mimeTypeOf(source.getName()) : mimeType;
        boolean isVideo = mime.startsWith("video/");
        boolean isImage = mime.startsWith("image/");
        boolean isAudio = mime.startsWith("audio/");

        final boolean isQ = Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q;
        Uri collection;
        if (isVideo) {
            collection = isQ ? MediaStore.Video.Media.getContentUri(MediaStore.VOLUME_EXTERNAL_PRIMARY)
                             : MediaStore.Video.Media.EXTERNAL_CONTENT_URI;
        } else if (isAudio) {
            collection = isQ ? MediaStore.Audio.Media.getContentUri(MediaStore.VOLUME_EXTERNAL_PRIMARY)
                             : MediaStore.Audio.Media.EXTERNAL_CONTENT_URI;
        } else {
            collection = isQ ? MediaStore.Images.Media.getContentUri(MediaStore.VOLUME_EXTERNAL_PRIMARY)
                             : MediaStore.Images.Media.EXTERNAL_CONTENT_URI;
        }

        ContentValues values = new ContentValues();
        values.put(MediaStore.MediaColumns.DISPLAY_NAME, source.getName());
        values.put(MediaStore.MediaColumns.MIME_TYPE, mime);
        if (isQ) {
            if (isVideo) {
                values.put(MediaStore.MediaColumns.RELATIVE_PATH, Environment.DIRECTORY_MOVIES);
            } else if (isAudio) {
                values.put(MediaStore.MediaColumns.RELATIVE_PATH, Environment.DIRECTORY_MUSIC);
            } else {
                values.put(MediaStore.MediaColumns.RELATIVE_PATH, Environment.DIRECTORY_PICTURES);
            }
        }

        ContentResolver resolver = context().getContentResolver();
        Uri target = resolver.insert(collection, values);
        if (target == null) {
            Log.e(TAG, "the MediaStore refused the insert for " + source.getName());
            return false;
        }

        try (InputStream in = new FileInputStream(source);
             OutputStream out = resolver.openOutputStream(target)) {
            if (out == null) {
                return false;
            }
            byte[] buffer = new byte[64 * 1024];
            int read;
            while ((read = in.read(buffer)) > 0) {
                out.write(buffer, 0, read);
            }
            return true;
        } catch (IOException e) {
            Log.e(TAG, "exporting " + localPath + " failed", e);
            resolver.delete(target, null, null);
            return false;
        }
    }

    @Nullable
    private static String displayNameOf(Uri uri) {
        try (Cursor cursor = context().getContentResolver()
                .query(uri, null, null, null, null)) {
            if (cursor != null && cursor.moveToFirst()) {
                int index = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME);
                if (index >= 0) {
                    return cursor.getString(index);
                }
            }
        } catch (Exception e) {
            Log.w(TAG, "reading the display name of " + uri + " failed", e);
        }
        return null;
    }

    private static String mimeTypeOf(String name) {
        String lower = name.toLowerCase();
        if (lower.endsWith(".png")) return "image/png";
        if (lower.endsWith(".jpg") || lower.endsWith(".jpeg")) return "image/jpeg";
        if (lower.endsWith(".tif") || lower.endsWith(".tiff")) return "image/tiff";
        if (lower.endsWith(".bmp")) return "image/bmp";
        if (lower.endsWith(".webp")) return "image/webp";
        if (lower.endsWith(".gif")) return "image/gif";
        if (lower.endsWith(".mp4")) return "video/mp4";
        if (lower.endsWith(".webm")) return "video/webm";
        if (lower.endsWith(".mov")) return "video/quicktime";
        if (lower.endsWith(".wav")) return "audio/wav";
        if (lower.endsWith(".mp3")) return "audio/mpeg";
        if (lower.endsWith(".ogg")) return "audio/ogg";
        if (lower.endsWith(".pdf")) return "application/pdf";
        if (lower.endsWith(".json")) return "application/json";
        if (lower.endsWith(".xml")) return "text/xml";
        if (lower.endsWith(".txt") || lower.endsWith(".env")) return "text/plain";
        return "application/octet-stream";
    }

    // =====================================================================
    //  Staging
    // =====================================================================

    private static File stagingDirectory() {
        File dir = new File(context().getCacheDir(), "saf-staging");
        if (!dir.isDirectory()) {
            //noinspection ResultOfMethodCallIgnored
            dir.mkdirs();
        }
        return dir;
    }

    // =====================================================================
    //  Pickers
    // =====================================================================

    public static String pickFile(String mimeFilter, boolean forWriting) {
        if (!requireInitialized()) {
            return "";
        }
        Intent intent = new Intent(forWriting ? Intent.ACTION_CREATE_DOCUMENT
                                              : Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType(mimeFilter == null || mimeFilter.isEmpty() ? "*/*" : mimeFilter);
        intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION
                | Intent.FLAG_GRANT_WRITE_URI_PERMISSION
                | Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION);
        Uri uri = runPicker(intent, forWriting ? REQUEST_CREATE_DOCUMENT : REQUEST_OPEN_DOCUMENT);

        if (uri == null) {
            return "";
        }
        if (!forWriting) {
            // Open the document straight away: the caller expects a path it can
            // hand to the file format readers.
            return materializeStandalone(uri, displayNameOf(uri));
        }
        return registerFileForWrite(uri);
    }

    public static String pickDirectory() {
        if (!requireInitialized()) {
            return "";
        }
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.LOLLIPOP) {
            return "";
        }
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
        intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION
                | Intent.FLAG_GRANT_WRITE_URI_PERMISSION
                | Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION
                | Intent.FLAG_GRANT_PREFIX_URI_PERMISSION);
        Uri uri = runPicker(intent, REQUEST_OPEN_TREE);
        if (uri == null) {
            return "";
        }
        String title = DocumentsContract.getTreeDocumentId(uri);
        int id = addTree(uri.toString(), title == null ? uri.toString() : title);
        if (id <= 0) {
            return "";
        }
        return new File(new File(getInternalRoot(), "saf"), String.valueOf(id)).getAbsolutePath();
    }

    /**
     * Registers a single document chosen for writing. Because the document is
     * not part of a granted tree the native side receives a writable staging
     * path and the content is pushed back through {@link #flushStandalone()}
     * when the save completes.
     */
    private static String registerFileForWrite(Uri uri) {
        String name = displayNameOf(uri);
        if (name == null || name.isEmpty()) {
            name = "document";
        }
        File staging = new File(stagingDirectory(), name);
        pendingWriteBack.put(staging.getAbsolutePath(), uri.toString());
        return staging.getAbsolutePath();
    }

    private static final java.util.concurrent.ConcurrentHashMap<String, String>
            pendingWriteBack = new java.util.concurrent.ConcurrentHashMap<>();

    /** Copies the staging file back to the document it came from. */
    public static boolean flushStandalone(String localPath) {
        if (!requireInitialized() || localPath == null) {
            return false;
        }
        String target = pendingWriteBack.get(localPath);
        if (target == null) {
            return false;
        }
        File source = new File(localPath);
        try (InputStream in = new FileInputStream(source);
             OutputStream out = context().getContentResolver()
                     .openOutputStream(Uri.parse(target), "wt")) {
            if (out == null) {
                return false;
            }
            byte[] buffer = new byte[64 * 1024];
            int read;
            while ((read = in.read(buffer)) > 0) {
                out.write(buffer, 0, read);
            }
            return true;
        } catch (IOException e) {
            Log.e(TAG, "writing back " + localPath + " failed", e);
            return false;
        }
    }

    private static String materializeStandalone(Uri uri, String name) {
        if (name == null || name.isEmpty()) {
            name = "document";
        }
        File staging = new File(stagingDirectory(), name);
        try (InputStream in = context().getContentResolver().openInputStream(uri);
             OutputStream out = new FileOutputStream(staging)) {
            if (in == null) {
                return "";
            }
            byte[] buffer = new byte[64 * 1024];
            int read;
            while ((read = in.read(buffer)) > 0) {
                out.write(buffer, 0, read);
            }
        } catch (IOException e) {
            Log.e(TAG, "materializing " + uri + " failed", e);
            return "";
        }
        return staging.getAbsolutePath();
    }

    private static Uri runPicker(final Intent intent, final int requestCode) {
        final Activity current = activity;
        if (current == null) {
            Log.e(TAG, "no activity is attached: the picker cannot be shown");
            return null;
        }

        final CountDownLatch latch = new CountDownLatch(1);
        pendingPickerLatch = latch;
        pendingPickerRequestCode = requestCode;
        pendingPickerResult.set(null);

        new Handler(Looper.getMainLooper()).post(new Runnable() {
            @Override
            public void run() {
                try {
                    current.startActivityForResult(intent, requestCode);
                } catch (ActivityNotFoundException e) {
                    Log.e(TAG, "no application can handle the picker", e);
                    pendingPickerResult.set(null);
                    latch.countDown();
                }
            }
        });

        try {
            if (!latch.await(PICKER_TIMEOUT_SECONDS, TimeUnit.SECONDS)) {
                Log.e(TAG, "the picker timed out");
                return null;
            }
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
            return null;
        } finally {
            pendingPickerLatch = null;
            pendingPickerRequestCode = -1;
        }
        return pendingPickerResult.get();
    }

    /** Called by the activity when a picker returns. */
    static void onPickerResult(int requestCode, int resultCode, @Nullable Intent data) {
        if (requestCode != REQUEST_OPEN_DOCUMENT
                && requestCode != REQUEST_CREATE_DOCUMENT
                && requestCode != REQUEST_OPEN_TREE) {
            return;
        }
        CountDownLatch latch = pendingPickerLatch;
        if (latch == null || requestCode != pendingPickerRequestCode) {
            return;
        }
        Uri uri = (resultCode == Activity.RESULT_OK && data != null) ? data.getData() : null;
        if (uri != null) {
            try {
                context().getContentResolver().takePersistableUriPermission(
                        uri, Intent.FLAG_GRANT_READ_URI_PERMISSION
                                | Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
            } catch (SecurityException e) {
                Log.w(TAG, "the picker result permission is not persistable", e);
            }
        }
        pendingPickerResult.set(uri);
        latch.countDown();
    }

    // =====================================================================
    //  Shell services
    // =====================================================================

    public static boolean openDocument(String path) {
        if (!requireInitialized() || path == null) {
            return false;
        }
        File file = new File(path);
        Uri uri = file.isFile() ? FileProviderSupport.uriFor(context(), file) : null;
        if (uri == null) {
            return false;
        }
        Intent intent = new Intent(Intent.ACTION_VIEW);
        intent.setDataAndType(uri, mimeTypeOf(file.getName()));
        intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_ACTIVITY_NEW_TASK);
        try {
            context().startActivity(intent);
            return true;
        } catch (ActivityNotFoundException e) {
            Log.w(TAG, "no application can open " + path, e);
            return false;
        }
    }

    public static boolean shareFile(String path, String mimeType) {
        if (!requireInitialized() || path == null) {
            return false;
        }
        File file = new File(path);
        Uri uri = FileProviderSupport.uriFor(context(), file);
        if (uri == null) {
            return false;
        }
        String mime = (mimeType == null || mimeType.isEmpty())
                ? mimeTypeOf(file.getName()) : mimeType;

        Intent intent = new Intent(Intent.ACTION_SEND);
        intent.setType(mime);
        intent.putExtra(Intent.EXTRA_STREAM, uri);
        intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
        Intent chooser = Intent.createChooser(intent, null);
        chooser.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        context().startActivity(chooser);
        return true;
    }

    public static void setKeepScreenOn(final boolean on) {
        final Activity current = activity;
        if (current == null) {
            return;
        }
        new Handler(Looper.getMainLooper()).post(new Runnable() {
            @Override
            public void run() {
                if (on) {
                    current.getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
                } else {
                    current.getWindow().clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
                }
            }
        });
    }

    public static void toast(final String message) {
        if (!requireInitialized() || message == null) {
            return;
        }
        new Handler(Looper.getMainLooper()).post(new Runnable() {
            @Override
            public void run() {
                Toast.makeText(context(), message, Toast.LENGTH_SHORT).show();
            }
        });
    }

    // =====================================================================
    //  Device information
    // =====================================================================

    public static boolean isLargeScreen() {
        if (!requireInitialized()) {
            return false;
        }
        Configuration configuration = context().getResources().getConfiguration();
        int size = configuration.screenLayout & Configuration.SCREENLAYOUT_SIZE_MASK;
        return size >= Configuration.SCREENLAYOUT_SIZE_LARGE;
    }

    public static int[] screenSize() {
        int[] out = new int[2];
        if (!requireInitialized()) {
            return out;
        }
        DisplayMetrics metrics = context().getResources().getDisplayMetrics();
        out[0] = Math.round(metrics.widthPixels / metrics.density);
        out[1] = Math.round(metrics.heightPixels / metrics.density);
        return out;
    }

    public static long internalStorageSizeKb() {
        if (!requireInitialized()) {
            return 0;
        }
        StatFs stat = new StatFs(context().getFilesDir().getAbsolutePath());
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR2) {
            return stat.getBlockCountLong() * stat.getBlockSizeLong() / 1024L;
        }
        return 0;
    }

    public static long internalStorageFreeKb() {
        if (!requireInitialized()) {
            return 0;
        }
        StatFs stat = new StatFs(context().getFilesDir().getAbsolutePath());
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR2) {
            return stat.getAvailableBlocksLong() * stat.getBlockSizeLong() / 1024L;
        }
        return 0;
    }

    // =====================================================================
    //  Helpers
    // =====================================================================

    private static void deleteRecursively(File file) {
        if (file == null || !file.exists()) {
            return;
        }
        File[] children = file.listFiles();
        if (children != null) {
            for (File child : children) {
                deleteRecursively(child);
            }
        }
        //noinspection ResultOfMethodCallIgnored
        file.delete();
    }

    private static void copyFile(File source, File target) throws IOException {
        try (InputStream in = new FileInputStream(source);
             OutputStream out = new FileOutputStream(target)) {
            byte[] buffer = new byte[64 * 1024];
            int read;
            while ((read = in.read(buffer)) > 0) {
                out.write(buffer, 0, read);
            }
        }
    }

    private static String readTextFile(File file) throws IOException {
        try (InputStream in = new FileInputStream(file)) {
            byte[] data = new byte[(int) Math.min(file.length(), 4096L)];
            int read = in.read(data);
            return read <= 0 ? "" : new String(data, 0, read, "UTF-8");
        }
    }

    private static void writeTextFile(File file, String text) throws IOException {
        try (OutputStream out = new FileOutputStream(file)) {
            out.write(text.getBytes("UTF-8"));
        }
    }

    // =====================================================================
    //  Native entry points
    // =====================================================================

    public static native void nativeSetVm();

    public static native void nativeRegisterActivity(Activity activity);

    public static native void nativeInitialize();
}
