#pragma once
#ifndef OT_ANDROID_JNI_H
#define OT_ANDROID_JNI_H

//=============================================================================
//
//  Internal JNI interface of the OpenToonz Android platform layer.
//
//  Everything declared here is Android-only: the desktop build compiles the
//  platform layer to inert stubs (see otandroid_jni.cpp) and never includes
//  this header.
//
//=============================================================================

#ifdef __ANDROID__

#include <jni.h>

#include <string>

namespace otandroid {

//! Identifiers of the cached Java methods; the enum keeps the accessor in
//! otandroid_jni.cpp in sync with the resolution performed in initialize().
enum StorageMethodId {
  SM_getInternalRoot,
  SM_getCacheRoot,
  SM_ensureAssetsExtracted,
  SM_listDirectory,
  SM_exists,
  SM_isDirectory,
  SM_createDirectory,
  SM_remove,
  SM_rename,
  SM_materialize,
  SM_store,
  SM_copyInto,
  SM_exportToMediaStore,
  SM_trees,
  SM_addTree,
  SM_removeTree,
  SM_pickFile,
  SM_pickDirectory,
  SM_openDocument,
  SM_shareFile,
  SM_keepScreenOn,
  SM_toast,
  SM_isLargeScreen,
  SM_screenSize,
  SM_internalStorageSizeKb,
  SM_internalStorageFreeKb
};

//! Attaches the calling thread to the Java VM if needed.  \p attached must be
//! passed back to detachEnv() when leaving the scope.
JNIEnv *attachEnv(bool &attached);
void detachEnv(JNIEnv *env, bool attached);

jclass storageClass();
jobject activityObject();
jmethodID storageMethod(StorageMethodId id);

std::string jstringToUtf8(JNIEnv *env, jstring s);
std::wstring jstringToWString(JNIEnv *env, jstring s);
jstring wstringToJstring(JNIEnv *env, const std::wstring &s);

//! True once the Java class references have been resolved successfully.
bool jniReady();

//! Records the Java VM (and, when given, the activity) handed over by the
//! framework.  Called from the JNI entry points in otandroid_storage.cpp.
void registerVmAndActivity(JNIEnv *env, jobject activity, bool vmOnly);

//! Looks up (once) the AndroidStorage helper class and its method ids.
void resolveStorageClass();
void resolveStorageMethods();

//! Creates the application storage layout and unpacks the bundled "stuff"
//! tree on first run.
void ensureStuffStaging();

}  // namespace otandroid

#endif  // __ANDROID__

#endif  // OT_ANDROID_JNI_H
