//=============================================================================
//
//  JNI plumbing for the OpenToonz Android platform layer.
//
//  All the platform services used by the application are implemented on the
//  Java side (see toonz/sources/android/java/).  This file owns the JavaVM
//  pointer and the cached class and method ids, and provides the marshalling
//  helpers the rest of the layer builds upon.
//
//  The class and method ids are resolved once, at startup: looking them up on
//  every call is one of the most common performance mistakes in JNI code, and
//  the platform layer is called from the rendering threads.
//
//=============================================================================

#include "otandroid.h"

#ifdef __ANDROID__

#include "otandroid_jni.h"

#include <QString>

#include <android/log.h>

namespace otandroid {

namespace {

JavaVM *g_vm           = nullptr;
jclass g_storageClass  = nullptr;
jclass g_activityClass = nullptr;
jobject g_activity     = nullptr;
bool g_initialized     = false;
bool g_methodsResolved = false;

struct StorageMethods {
  jmethodID getInternalRoot        = nullptr;
  jmethodID getCacheRoot           = nullptr;
  jmethodID ensureAssetsExtracted  = nullptr;
  jmethodID listDirectory          = nullptr;
  jmethodID exists                 = nullptr;
  jmethodID isDirectory            = nullptr;
  jmethodID createDirectory        = nullptr;
  jmethodID remove                 = nullptr;
  jmethodID rename                 = nullptr;
  jmethodID materialize            = nullptr;
  jmethodID store                  = nullptr;
  jmethodID copyInto               = nullptr;
  jmethodID exportToMediaStore     = nullptr;
  jmethodID trees                  = nullptr;
  jmethodID addTree                = nullptr;
  jmethodID removeTree             = nullptr;
  jmethodID pickFile               = nullptr;
  jmethodID pickDirectory          = nullptr;
  jmethodID openDocument           = nullptr;
  jmethodID shareFile              = nullptr;
  jmethodID keepScreenOn           = nullptr;
  jmethodID toast                  = nullptr;
  jmethodID isLargeScreen          = nullptr;
  jmethodID screenSize             = nullptr;
  jmethodID internalStorageSizeKb  = nullptr;
  jmethodID internalStorageFreeKb  = nullptr;
};

StorageMethods g_m;

std::wstring fromJString(jstring s, JNIEnv *env) {
  if (!s) return std::wstring();
  const jsize len    = env->GetStringLength(s);
  const jchar *chars = env->GetStringChars(s, nullptr);
  std::wstring out;
  if (chars) {
    out.reserve(static_cast<size_t>(len));
    for (jsize i = 0; i < len; ++i)
      out.push_back(static_cast<wchar_t>(chars[i]));
    env->ReleaseStringChars(s, chars);
  }
  return out;
}

jstring toJString(JNIEnv *env, const std::wstring &s) {
  return env->NewString(reinterpret_cast<const jchar *>(s.c_str()),
                        static_cast<jsize>(s.size()));
}

//! Resolves a method declared either as static or as an instance method and
//! clears the pending exception when the lookup fails, so that a missing
//! method degrades to "feature unavailable" instead of aborting the process.
jmethodID resolveMethod(JNIEnv *env, jclass cls, const char *name,
                        const char *sig) {
  jmethodID id = env->GetStaticMethodID(cls, name, sig);
  if (!id) {
    env->ExceptionClear();
    id = env->GetMethodID(cls, name, sig);
    if (!id) {
      env->ExceptionClear();
      __android_log_print(ANDROID_LOG_WARN, "OpenToonz",
                          "AndroidStorage.%s%s could not be resolved", name,
                          sig);
    }
  }
  return id;
}

}  // namespace

//=============================================================================
//  Accessors used by the rest of the platform layer
//=============================================================================

JNIEnv *attachEnv(bool &attached) {
  attached = false;
  if (!g_vm) return nullptr;
  JNIEnv *env = nullptr;
  const jint rc = g_vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6);
  if (rc == JNI_EDETACHED) {
    if (g_vm->AttachCurrentThread(&env, nullptr) != JNI_OK) return nullptr;
    attached = true;
  } else if (rc != JNI_OK) {
    return nullptr;
  }
  return env;
}

void detachEnv(JNIEnv *, bool attached) {
  if (attached && g_vm) g_vm->DetachCurrentThread();
}

jclass storageClass() { return g_storageClass; }
jobject activityObject() { return g_activity; }

jmethodID storageMethod(StorageMethodId id) {
  switch (id) {
  case SM_getInternalRoot: return g_m.getInternalRoot;
  case SM_getCacheRoot: return g_m.getCacheRoot;
  case SM_ensureAssetsExtracted: return g_m.ensureAssetsExtracted;
  case SM_listDirectory: return g_m.listDirectory;
  case SM_exists: return g_m.exists;
  case SM_isDirectory: return g_m.isDirectory;
  case SM_createDirectory: return g_m.createDirectory;
  case SM_remove: return g_m.remove;
  case SM_rename: return g_m.rename;
  case SM_materialize: return g_m.materialize;
  case SM_store: return g_m.store;
  case SM_copyInto: return g_m.copyInto;
  case SM_exportToMediaStore: return g_m.exportToMediaStore;
  case SM_trees: return g_m.trees;
  case SM_addTree: return g_m.addTree;
  case SM_removeTree: return g_m.removeTree;
  case SM_pickFile: return g_m.pickFile;
  case SM_pickDirectory: return g_m.pickDirectory;
  case SM_openDocument: return g_m.openDocument;
  case SM_shareFile: return g_m.shareFile;
  case SM_keepScreenOn: return g_m.keepScreenOn;
  case SM_toast: return g_m.toast;
  case SM_isLargeScreen: return g_m.isLargeScreen;
  case SM_screenSize: return g_m.screenSize;
  case SM_internalStorageSizeKb: return g_m.internalStorageSizeKb;
  case SM_internalStorageFreeKb: return g_m.internalStorageFreeKb;
  }
  return nullptr;
}

std::string jstringToUtf8(JNIEnv *env, jstring s) {
  return QString::fromStdWString(fromJString(s, env)).toUtf8().toStdString();
}

std::wstring jstringToWString(JNIEnv *env, jstring s) {
  return fromJString(s, env);
}

jstring wstringToJstring(JNIEnv *env, const std::wstring &s) {
  return toJString(env, s);
}

bool jniReady() { return g_initialized && g_vm && g_storageClass; }

//=============================================================================
//  Registration of the Java VM and activity
//=============================================================================

void registerVmAndActivity(JNIEnv *env, jobject activity, bool vmOnly) {
  if (env) env->GetJavaVM(&g_vm);

  if (vmOnly || !activity) return;

  if (g_activity) {
    // The activity is recreated on configuration changes; the stale global
    // reference has to be released before adopting the new one.
    bool attached = false;
    JNIEnv *e     = attachEnv(attached);
    if (e) {
      e->DeleteGlobalRef(g_activity);
      detachEnv(e, attached);
    }
    g_activity = nullptr;
  }

  bool attached = false;
  JNIEnv *e     = env ? env : attachEnv(attached);
  if (!e) return;

  g_activity = e->NewGlobalRef(activity);

  if (!g_activityClass) {
    jclass cls = e->GetObjectClass(activity);
    if (cls) {
      g_activityClass = static_cast<jclass>(e->NewGlobalRef(cls));
      e->DeleteLocalRef(cls);
    }
  }

  if (!env) detachEnv(e, attached);
}

//! Resolves the storage helper class; called once by initialize().
void resolveStorageClass() {
  if (!g_vm || g_storageClass) return;

  bool attached = false;
  JNIEnv *env   = attachEnv(attached);
  if (!env) return;

  jclass cls = env->FindClass("io/github/opentoonz/android/AndroidStorage");
  if (cls) {
    g_storageClass = static_cast<jclass>(env->NewGlobalRef(cls));
    env->DeleteLocalRef(cls);
  } else {
    env->ExceptionClear();
    __android_log_print(ANDROID_LOG_ERROR, "OpenToonz",
                        "io.github.opentoonz.android.AndroidStorage is missing "
                        "from the APK");
  }

  detachEnv(env, attached);
}

//! Binds every method id used by the platform layer.
void resolveStorageMethods() {
  if (!g_storageClass || g_methodsResolved) return;
  g_methodsResolved = true;

  bool attached = false;
  JNIEnv *env   = attachEnv(attached);
  if (!env) return;

  g_m.getInternalRoot = resolveMethod(env, g_storageClass, "getInternalRoot",
                                      "()Ljava/lang/String;");
  g_m.getCacheRoot = resolveMethod(env, g_storageClass, "getCacheRoot",
                                   "()Ljava/lang/String;");
  g_m.ensureAssetsExtracted = resolveMethod(
      env, g_storageClass, "ensureAssetsExtracted", "(Ljava/lang/String;)Z");
  g_m.listDirectory = resolveMethod(env, g_storageClass, "listDirectory",
                                    "(Ljava/lang/String;)[Ljava/lang/String;");
  g_m.exists =
      resolveMethod(env, g_storageClass, "exists", "(Ljava/lang/String;)Z");
  g_m.isDirectory = resolveMethod(env, g_storageClass, "isDirectory",
                                  "(Ljava/lang/String;)Z");
  g_m.createDirectory = resolveMethod(env, g_storageClass, "createDirectory",
                                      "(Ljava/lang/String;)Z");
  g_m.remove =
      resolveMethod(env, g_storageClass, "remove", "(Ljava/lang/String;Z)Z");
  g_m.rename = resolveMethod(env, g_storageClass, "rename",
                             "(Ljava/lang/String;Ljava/lang/String;)Z");
  g_m.materialize = resolveMethod(env, g_storageClass, "materialize",
                                  "(Ljava/lang/String;)Ljava/lang/String;");
  g_m.store = resolveMethod(env, g_storageClass, "store",
                            "(Ljava/lang/String;Ljava/lang/String;)Z");
  g_m.copyInto = resolveMethod(env, g_storageClass, "copyInto",
                               "(Ljava/lang/String;Ljava/lang/String;)Z");
  g_m.exportToMediaStore =
      resolveMethod(env, g_storageClass, "exportToMediaStore",
                    "(Ljava/lang/String;Ljava/lang/String;)Z");
  g_m.trees =
      resolveMethod(env, g_storageClass, "trees", "()[Ljava/lang/String;");
  g_m.addTree = resolveMethod(env, g_storageClass, "addTree",
                              "(Ljava/lang/String;Ljava/lang/String;)I");
  g_m.removeTree = resolveMethod(env, g_storageClass, "removeTree", "(I)V");
  g_m.pickFile = resolveMethod(env, g_storageClass, "pickFile",
                               "(Ljava/lang/String;Z)Ljava/lang/String;");
  g_m.pickDirectory = resolveMethod(env, g_storageClass, "pickDirectory",
                                    "()Ljava/lang/String;");
  g_m.openDocument = resolveMethod(env, g_storageClass, "openDocument",
                                   "(Ljava/lang/String;)Z");
  g_m.shareFile = resolveMethod(env, g_storageClass, "shareFile",
                                "(Ljava/lang/String;Ljava/lang/String;)Z");
  g_m.keepScreenOn = resolveMethod(env, g_storageClass, "setKeepScreenOn",
                                   "(Z)V");
  g_m.toast = resolveMethod(env, g_storageClass, "toast", "(Ljava/lang/String;)V");
  g_m.isLargeScreen = resolveMethod(env, g_storageClass, "isLargeScreen", "()Z");
  g_m.screenSize = resolveMethod(env, g_storageClass, "screenSize", "()[I");
  g_m.internalStorageSizeKb =
      resolveMethod(env, g_storageClass, "internalStorageSizeKb", "()J");
  g_m.internalStorageFreeKb =
      resolveMethod(env, g_storageClass, "internalStorageFreeKb", "()J");

  detachEnv(env, attached);
}

}  // namespace otandroid

#endif  // __ANDROID__
