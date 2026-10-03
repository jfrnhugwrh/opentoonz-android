# OpenToonz for Android

This directory contains everything the Android port adds to OpenToonz. The rest
of the tree is the upstream application, and the guiding rule of the port is
that it stays that way: **the behaviour of OpenToonz is unchanged**. The
mechanics, the rendering, the animation tools, the workflows and the file
formats are the ones the desktop builds use — what differs is only the parts
that cannot exist on Android, plus the way the interface is presented.

```
android/
├── gl_compat/     OpenGL ES compatibility layer (fixed function pipeline, GLU, GLUT, GLEW)
├── platform/      JNI bridge to the platform services (scoped storage, SAF, MediaStore)
├── java/          the Java half of the platform layer
├── gradle/        the Android application project
├── android_thirdparty.cmake   third party libraries built from the bundled sources
└── CMakeLists.txt             the two support libraries
```

---

## 1. What had to be replaced, and what did not

### The rendering core is untouched

The TnzCore rendering core — inherited from Toonz 4.x — draws through the
desktop OpenGL fixed function pipeline: immediate mode (`glBegin`/`glEnd`), the
matrix stacks, display lists, the imaging subset, and GLU tessellation for every
filled vector region. None of that exists in OpenGL ES.

Rather than rewriting the renderer — which would have changed how the
application draws — `gl_compat/` **re-implements those entry points on top of
OpenGL ES 3.0**:

| Area | What the shim provides |
|---|---|
| Immediate mode | Recorded per `glBegin`/`glEnd` and submitted as batched vertex arrays to an internal shader |
| Matrix stacks | Shadow copies of the modelview / projection / texture stacks, applied as one MVP uniform |
| Display lists | Serialised into a dictionary of recorded geometry, replayed on `glCallList` |
| Texture environments | `GL_MODULATE`, `GL_REPLACE`, `GL_DECAL` reproduced in the fragment shader |
| Alpha test, colour mask, blending | Tracked in the shim and applied per draw |
| GLU | Tessellator (winding rules, contour orientation, holes, boundary mode), quadrics, `gluProject`/`gluUnProject`, `gluOrtho2D`, `gluScaleImage` |
| GLUT | The stroke / bitmap font subset `tglDrawText` needs, as a single stroke face |
| GLEW | A thin shim so the `GLEW_*` capability tests in the effects keep working |

The parts of the desktop pipeline that OpenGL ES simply does not have —
PBuffer offscreen contexts, GLX, `QGLFormat`, the legacy `QGLContext` — are
replaced by their modern equivalents (`QOpenGLContext` on a `QOffscreenSurface`
rendering into a framebuffer object), which is what the desktop Linux build
already does for its own `QtOfflineGL` path.

The shim is verified by a host test (`gl_compat/test/`) that exercises the
tessellator — including a square with a hole and a concave outline — and the
projection maths, so a regression is caught without a device. The
`gl-compat-test` job of the Android workflow runs it on every manual run.

### Everything else is the desktop code

Concretely, of the application:

* the whole of `tnzcore`, `tnzbase`, `tnzext`, `toonzlib`, `stdfx`, `colorfx`,
  `image`, `sound`, `toonzqt` and `tnztools` is the same code, built for the
  NDK toolchain;
* the scene format, the level formats (`.tlv`, `.pli`, `.tzp`, `.tzu`, `.psd`,
  `.sxd`, …), the palettes, the effects, the rooms and the preferences are
  unchanged — the Android build writes and reads exactly the same files;
* the tools, the xsheet, the timeline, the function editor, the schematic, the
  plastic deformer and the render pipeline behave identically.

Where the platform genuinely has no counterpart, the *subsystem* is replaced
while keeping its contract:

| Subsystem | Desktop | Android | Why |
|---|---|---|---|
| Offscreen GL | GLX / WGL / PBuffer | Qt offscreen surface + FBO | no GLX, no PBuffer in ES |
| LZO raster codec | `lzocompress` / `lzodecompress` helper executables | minilzo linked in process | an APK cannot spawn helper binaries; the **stream format is byte for byte the same** |
| Camera capture | Canon SDK / libusb / OpenCV | the Qt Multimedia capture path (`*_qt.cpp`) | there is no Canon SDK or libusb on Android; the capture workflow and the levels produced are the same |
| Batch tools, render farm | `tcleanup`, `tcomposer`, `tfarm…` executables | not built | an APK has a single native entry point; the commands they implement remain available in the application |
| Printing | Qt PrintSupport | not built | Qt for Android has no print support; every other export path, including the PDF export, is unchanged |
| MyPaint brush simulation | libmypaint | graceful fallback | the bundled copy is a Windows build; the `.myb` files are still read and written |
| Crash backtrace | `execinfo` + `addr2line` | platform crash reporter | bionic has no `execinfo` |

---

## 2. Scoped storage

Android 10 and later forbid opening arbitrary paths in shared storage. The port
therefore uses the two storage models the platform provides, and **declares no
storage permission at all**:

**Application specific storage** — `<internal>/files/stuff` is the working root.
It needs no permission, is removed with the application, and holds the stuff
tree, the projects, the cache and the configuration. On first run the bundled
`stuff` tree is unpacked from the APK assets by
`AndroidStorage.ensureAssetsExtracted()`, which is the Android equivalent of the
desktop install step.

**Scoped storage through the SAF** — everything outside the application's own
directories is reached through the Storage Access Framework. Since the whole
core works with `TFilePath`, the granted trees are mapped into a synthetic
directory:

```
<internal>/saf/<treeId>/<path inside the granted tree>
```

The `saf` component is reserved and is **never created on disk**, so the native
side recognises a mapped path and routes the operation to the platform layer.
That mapping is what lets the unmodified core — the file browser, the level
loaders, the scene builder — read and write documents the user picked:

| `TFilePath` operation | Platform call |
|---|---|
| `TFileStatus(path)` | `DocumentFile` existence / directory query |
| `TSystem::readDirectory` | `DocumentFile.listFiles()` |
| `TSystem::mkDir` / `deleteFile` / `rmDirTree` | `DocumentFile.createDirectory()` / `delete()` |
| `TSystem::copyFile` / `renameFile` | stream copy / `DocumentFile.renameTo()` |
| opening a level or a scene | the document is staged into the application cache, then read normally |
| saving | the staged file is streamed back into the document |

Media that has to become visible to other applications (rendered frames,
exported movies) is published through the **MediaStore**, which is the supported
way of writing to the shared collections on Android 10+:
`otandroid::saf::exportToMediaStore()`.

Picking a file, picking a folder and creating a document are handled by
`AndroidStorage.pickFile()`, `pickDirectory()`; the activity delivers the
results to the native thread waiting for them.

---

## 3. The touch interface

The desktop interface assumes a mouse, a keyboard and a wide window. The port
changes the *presentation* without changing what anything does — every touch
control triggers the same `CommandManager` action the desktop menu item or
shortcut triggers.

* **`AndroidCommandBar`** replaces the menu bar with a single strip holding
  *Home / Back / Rooms / Commands*. The command list is built from the menu bar
  of the current room, searchable, and with finger sized rows. The room picker
  replaces the room tabs.
* **`AndroidTouchPanel`** is a thumb reachable strip (a room panel) with the
  commands a drawing session needs most: undo/redo, the drawing tools, onion
  skin, frame navigation, zoom and fit.
* **`AndroidSceneViewGestureHandler`** switches the viewer's multi touch
  navigation on by default (it is behind a command on the desktop), and turns a
  long press into the context menu the right mouse button opens on the desktop.
* **Room layouts**: `stuff/profiles/layouts/rooms/Android/` defines four rooms
  designed for a small display (Draw, Timeline, Palette, Files) and is the
  default room choice on Android.
* **Stylesheet**: `stuff/config/qss/Android/android.qss` raises every
  interactive control to at least 48 dp and widens the scrollbars and splitters.
  It is layered on top of the selected colour scheme, so the desktop themes keep
  working.

---

## 4. Building

### Prerequisites

* Android NDK r25 or later
* Qt for Android 5.15.2 (arm64-v8a, armeabi-v7a or x86_64), plus the host Qt
  tools (`moc`, `rcc`)
* CMake 3.21+, Ninja, JDK 17, Android SDK (platform 33, build-tools 33.0.2)

### Configure and build

```bash
NDK=/path/to/ndk/25.2.9519653
QT=/path/to/Qt/5.15.2/android

cmake -S toonz/sources -B build-android -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=$NDK/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-24 \
  -DANDROID_STL=c++_shared \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH=$QT \
  -DQt5_DIR=$QT/lib/cmake/Qt5

cmake --build build-android --parallel
```

Boost is header-only for OpenToonz; the build looks for a system copy first and
fetches the official release archive otherwise (override the version with
`-DOT_BOOST_VERSION=1.87.0`, or point `-DBOOST_ROOT=` at a local copy).

### Package the APK

```bash
mkdir -p toonz/sources/android/gradle/app/src/main/jniLibs/arm64-v8a
cp build-android/lib/*.so toonz/sources/android/gradle/app/src/main/jniLibs/arm64-v8a/
cp -r stuff toonz/sources/android/gradle/app/src/main/assets/
cd toonz/sources/android/gradle
echo "sdk.dir=$ANDROID_SDK_ROOT" > local.properties
gradle assembleRelease
```

### Run the compatibility layer test on a normal machine

```bash
cmake -S toonz/sources/android/gl_compat/test -B build/gl_compat_test
cmake --build build/gl_compat_test
./build/gl_compat_test/ot_gl_compat_test
```

---

## 5. Continuous integration

`.github/workflows/workflow_android.yml` builds and tests the port. It is
**manual only** — `workflow_dispatch`, with no `push`, `pull_request` or
`schedule` trigger — because a full Android build takes a long time and should
not run on every commit:

```bash
gh workflow run workflow_android.yml -f abi=arm64-v8a
```

It runs two jobs:

1. `gl-compat-test` — builds and runs the host smoke test of the compatibility
   layer. About a minute, and it needs no device.
2. `android` — cross compiles the application, stages the Qt runtime and the
   `stuff` assets, packages the APK and uploads it (together with the native
   libraries and, on failure, the CMake logs) as artifacts.

---

## 6. Mobile-specific notes

* **Performance.** The Android build does not enable the SSE2 fast paths of
  `trop`, because they are guarded by `_WIN32 && x64`. The scalar
  implementations are used unchanged, so the output is identical; a NEON port of
  `TRop::resample` and `tblur` would be a pure optimisation and is left for a
  follow-up.
* **Memory.** The manifest sets `largeHeap`, and the core's own memory
  management (`TBigMemoryManager`, the image cache) is unchanged. On low memory
  devices the cache size in Preferences is what has to be lowered.
* **No storage permission.** If a future change needs one, it has to be a
  scoped one: the port deliberately avoids `MANAGE_EXTERNAL_STORAGE` and the
  legacy `WRITE_EXTERNAL_STORAGE`.
