package io.github.opentoonz.android;

import android.content.Context;
import android.net.Uri;

import androidx.core.content.FileProvider;

import java.io.File;

import android.content.pm.ProviderInfo;

/**
 * Shareable URIs for files that live in the application specific storage.
 *
 * <p>Since Android 7 applications may not hand {@code file://} URIs to other
 * applications: a content provider has to serve them instead. The provider is
 * declared in the manifest and this helper is the single place that builds
 * such URIs, so the rest of the platform layer does not have to care about
 * which storage a document came from.
 */
final class FileProviderSupport {

    /** Authority used by the provider declared in AndroidManifest.xml. */
    private static final String AUTHORITY_SUFFIX = ".fileprovider";

    private FileProviderSupport() {
    }

    static Uri uriFor(Context context, File file) {
        if (file == null || !file.exists()) {
            return null;
        }
        try {
            String authority = context.getPackageName() + AUTHORITY_SUFFIX;
            return FileProvider.getUriForFile(context, authority, file);
        } catch (IllegalArgumentException e) {
            // The file is outside the paths the provider exposes: fall back to
            // the SAF document when possible, otherwise there is nothing that
            // can be shared.
            return null;
        }
    }

    /** True when the provider is present in the manifest. */
    static boolean isConfigured(Context context) {
        ProviderInfo info = context.getPackageManager().resolveContentProvider(
                context.getPackageName() + AUTHORITY_SUFFIX, 0);
        return info != null;
    }
}
