package io.github.opentoonz.android;

import android.content.Intent;
import android.content.res.Configuration;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.WindowManager;

import org.qtproject.qt5.android.bindings.QtActivity;

/**
 * Main activity of the OpenToonz Android port.
 *
 * <p>The activity is deliberately thin. Everything OpenToonz does is done by
 * the QApplication running inside this activity, exactly as on the desktop; the
 * activity only supplies what the Android platform requires:
 *
 * <ul>
 *   <li>it hands the Java VM and the activity reference to the native platform
 *       layer, so that native threads can reach the storage services;</li>
 *   <li>it routes the results of the Storage Access Framework pickers back to
 *       the native code waiting for them;</li>
 *   <li>it applies the window flags a drawing application needs - a full screen
 *       editing surface that keeps the display awake while rendering.</li>
 * </ul>
 */
public class OpenToonzActivity extends QtActivity {

    private static final String TAG = "OpenToonz";

    /** Set by the native side when a long running operation starts. */
    private static volatile boolean rendering = false;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        AndroidStorage.attach(this);
        AndroidStorage.attachActivity(this);
        AndroidStorage.nativeSetVm();

        super.onCreate(savedInstanceState);

        AndroidStorage.nativeRegisterActivity(this);
        AndroidStorage.nativeInitialize();

        // The viewer draws the whole UI itself; letting the system draw a
        // status bar over it would steal horizontal space from the canvas.
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        Log.i(TAG, "OpenToonz activity created");
    }

    @Override
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        setIntent(intent);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        // The SAF pickers are called synchronously from native rendering
        // threads; the result has to be delivered before the framework's own
        // handling so that the waiting thread is unblocked promptly.
        AndroidStorage.onPickerResult(requestCode, resultCode, data);
        super.onActivityResult(requestCode, resultCode, data);
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        // The Qt surface is recreated by the framework; the rooms are re-laid
        // out by the native side when it receives the resize event.
    }

    @Override
    protected void onDestroy() {
        AndroidStorage.attachActivity(null);
        super.onDestroy();
    }

    /** Called from the native layer while a render is in progress. */
    public static void setRendering(boolean value) {
        rendering = value;
    }

    public static boolean isRendering() {
        return rendering;
    }

    /**
     * Hides the system UI: the viewer covers the entire display, which is what
     * a drawing application wants on a phone.
     */
    @SuppressWarnings("deprecation")
    static void enterImmersiveMode(final QtActivity activity) {
        if (activity == null || Build.VERSION.SDK_INT < Build.VERSION_CODES.KITKAT) {
            return;
        }
        final View decor = activity.getWindow().getDecorView();
        decor.setSystemUiVisibility(View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_FULLSCREEN
                | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);
    }
}
