#pragma once
#ifndef OT_ANDROID_H
#define OT_ANDROID_H

//=============================================================================
//
//  OpenToonz Android platform layer.
//
//  Android differs from the desktop platforms in three ways that the core
//  cannot express by itself:
//
//   1. The application is not installed into a browsable directory tree: the
//      default "stuff" tree must be unpacked from the APK assets into the
//      app specific internal storage on first run.
//
//   2. Since Android 10 applications may not open arbitrary paths in shared
//      storage.  Documents are reached through the Storage Access Framework
//      (SAF), which hands out opaque document URIs rather than file paths.
//
//   3. Some facilities the core relies on (trash, shared libraries directories,
//      user accounts) either do not exist or work differently.
//
//  This header exposes the bridge used by the rest of the code base.  It is
//  deliberately small: everything that needs platform services goes through
//  here, which keeps the number of #ifdef ANDROID branches in the core low.
//
//=============================================================================

#include "tfilepath.h"

#include <string>
#include <vector>

namespace otandroid {

//! Number of the JNI interface version this header exposes.
#define OT_ANDROID_API_VERSION 1

//=============================================================================
//  Lifetime
//=============================================================================

//! Called once, right after QApplication has been constructed.  Caches the
//! JavaVM and the activity references and prepares the storage layout.
void initialize();

//! True once initialize() succeeded; every other entry point is a no-op
//! otherwise, so the desktop code paths can call them unconditionally.
bool isAvailable();

//=============================================================================
//  Application specific storage (never requires a permission)
//=============================================================================

//! /data/data/<package>/files - the internal, path accessible storage.
TFilePath getInternalRoot();

//! <internal>/stuff - the working root handed to TEnv::setStuffDir().
TFilePath getStuffRoot();

//! Directory used for scratch data (multi layer PSD staging, ffmpeg caches...).
TFilePath getCacheRoot();

//! Unpacks the "stuff" tree bundled in the APK assets, once.  Returns true when
//! the tree is present (either already unpacked or extracted successfully).
bool ensureStuffExtracted();

//=============================================================================
//  Scoped storage / Storage Access Framework
//=============================================================================

namespace saf {

//! Root of the synthetic tree the SAF documents are mapped into.  A path is a
//! SAF path when it lives under this directory, which never exists on disk.
TFilePath root();

//! True when \p path refers to a document reached through the SAF.
bool isSafPath(const TFilePath &path);

//! Resource ids of the storage volumes the user has granted access to.
struct TreeInfo {
  int id;                 //!< Numeric id used in the path mapping
  std::wstring treeUri;   //!< The persisted ACTION_OPEN_DOCUMENT_TREE uri
  std::wstring title;     //!< Human readable name shown in the file browser
};
std::vector<TreeInfo> trees();

//! Registers a tree uri (already persisted by the framework) and returns its id.
int addTree(const std::wstring &treeUri, const std::wstring &title);
void removeTree(int id);

//! Directory listing.  \p dirs and \p files receive the child names.
bool listDirectory(const TFilePath &path, std::vector<std::wstring> &dirs,
                   std::vector<std::wstring> &files);

bool exists(const TFilePath &path);
bool isDirectory(const TFilePath &path);
bool createDirectory(const TFilePath &path);
bool remove(const TFilePath &path, bool recursive);
bool rename(const TFilePath &src, const TFilePath &dst);

//! Copies a SAF document to a local file.  Returns an empty path on failure.
TFilePath materialize(const TFilePath &safPath);

//! Writes \p localFile back into the SAF document at \p safPath.
bool store(const TFilePath &safPath, const TFilePath &localFile);

//! Copies a local file or directory into a SAF directory.
bool copyInto(const TFilePath &localSource, const TFilePath &safDirectory);

//! Exports a local file to the shared MediaStore collection.  Used for the
//! "Save to gallery" / share actions.
bool exportToMediaStore(const TFilePath &localFile,
                        const std::wstring &mimeType);

//! Launches the SAF "create document" flow.  Returns the chosen path, or an
//! empty path when the user cancels.  Blocking: the UI thread runs the picker.
TFilePath pickFile(const std::wstring &mimeFilter, bool forWriting);
TFilePath pickDirectory();

}  // namespace saf

//=============================================================================
//  Shell / utility services
//=============================================================================

//! Opens a file with the system handler (Intent ACTION_VIEW).
bool openDocument(const TFilePath &path);

//! Shares a file through the system share sheet.
bool shareFile(const TFilePath &path, const std::wstring &mimeType);

//! Keeps the screen on while a long render is running.
void setKeepScreenOn(bool on);

//! Shows a transient message at the bottom of the screen.
void showToast(const std::wstring &message);

//! Writes a line to the Android log (used instead of stderr).
void logMessage(const char *tag, const char *message);

//=============================================================================
//  Device information
//=============================================================================

//! Total size of the internal volume, in kilobytes.
long long internalStorageSizeKb();

//! Free size of the internal volume, in kilobytes.
long long internalStorageFreeKb();

//! True when the device is a tablet / large screen, used to pick the default
//! room layout.
bool isLargeScreen();

//! Current screen size in device independent pixels.
void screenSize(int &widthDp, int &heightDp);

}  // namespace otandroid

#endif  // OT_ANDROID_H
