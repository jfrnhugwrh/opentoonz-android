//=============================================================================
//
//  OpenToonz Android platform layer - storage services.
//
//  Two storage models coexist on Android and this file implements both:
//
//   * Application specific storage (<internal>/files): a real, path accessible
//     directory that needs no permission and is removed with the application.
//     The OpenToonz working tree - "stuff", projects, cache - lives here.  The
//     bundled stuff tree is unpacked from the APK assets on first run, which is
//     the Android equivalent of the desktop install step.
//
//   * Scoped storage: everything outside the application's own directories is
//     reachable only through the Storage Access Framework, which hands out
//     document URIs.  To keep the whole core working with TFilePath - the file
//     format loaders, the scene builder, the file browser - the granted trees
//     are mapped into a synthetic directory:
//
//         <internal>/saf/<treeId>/<path inside the tree>
//
//     The synthetic directory never exists on disk (the "<internal>/saf"
//     component is reserved and never created), so every operation on it is
//     intercepted here and translated into a DocumentFile call.
//
//=============================================================================

#include "otandroid.h"

#ifdef __ANDROID__

#include "otandroid_jni.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QString>

#include <android/log.h>

#include <algorithm>
#include <mutex>

namespace otandroid {

namespace {

std::once_flag g_initOnce;
bool g_available = false;

TFilePath g_internalRoot;
TFilePath g_stuffRoot;
TFilePath g_cacheRoot;

//! Calls a Java method returning a String, with one String argument.
std::wstring callStringMethod(StorageMethodId id, const std::wstring &arg) {
  if (!jniReady()) return std::wstring();
  jmethodID mid = storageMethod(id);
  if (!mid) return std::wstring();

  bool attached = false;
  JNIEnv *env   = attachEnv(attached);
  if (!env) return std::wstring();

  jstring jarg = wstringToJstring(env, arg);
  jobject result =
      env->CallStaticObjectMethod(storageClass(), mid, jarg);
  env->DeleteLocalRef(jarg);

  std::wstring out;
  if (env->ExceptionCheck()) {
    env->ExceptionDescribe();
    env->ExceptionClear();
  } else if (result) {
    out = jstringToWString(env, static_cast<jstring>(result));
    env->DeleteLocalRef(result);
  }

  detachEnv(env, attached);
  return out;
}

bool callBooleanMethod(StorageMethodId id, const std::wstring &arg) {
  if (!jniReady()) return false;
  jmethodID mid = storageMethod(id);
  if (!mid) return false;

  bool attached = false;
  JNIEnv *env   = attachEnv(attached);
  if (!env) return false;

  jstring jarg   = wstringToJstring(env, arg);
  jboolean result = env->CallStaticBooleanMethod(storageClass(), mid, jarg);
  env->DeleteLocalRef(jarg);

  if (env->ExceptionCheck()) {
    env->ExceptionDescribe();
    env->ExceptionClear();
    result = JNI_FALSE;
  }

  detachEnv(env, attached);
  return result == JNI_TRUE;
}

bool callBooleanMethod2(StorageMethodId id, const std::wstring &a,
                        const std::wstring &b) {
  if (!jniReady()) return false;
  jmethodID mid = storageMethod(id);
  if (!mid) return false;

  bool attached = false;
  JNIEnv *env   = attachEnv(attached);
  if (!env) return false;

  jstring ja      = wstringToJstring(env, a);
  jstring jb      = wstringToJstring(env, b);
  jboolean result = env->CallStaticBooleanMethod(storageClass(), mid, ja, jb);
  env->DeleteLocalRef(ja);
  env->DeleteLocalRef(jb);

  if (env->ExceptionCheck()) {
    env->ExceptionDescribe();
    env->ExceptionClear();
    result = JNI_FALSE;
  }

  detachEnv(env, attached);
  return result == JNI_TRUE;
}

//-------------------------------------------------------------------------
//  SAF path mapping
//-------------------------------------------------------------------------

//! Splits a SAF mapped path into the tree id and the relative path inside it.
bool splitSafPath(const TFilePath &path, int &treeId, std::wstring &relative) {
  const TFilePath root      = saf::root();
  const std::wstring rootStr = root.getWideString();
  const std::wstring pathStr = path.getWideString();

  if (pathStr.size() <= rootStr.size()) return false;
  if (pathStr.compare(0, rootStr.size(), rootStr) != 0) return false;

  std::wstring rest = pathStr.substr(rootStr.size());
  while (!rest.empty() && (rest[0] == L'/' || rest[0] == L'\\')) rest.erase(0, 1);
  if (rest.empty()) return false;

  const size_t sep = rest.find_first_of(L"/\\");
  const std::wstring idStr = (sep == std::wstring::npos) ? rest : rest.substr(0, sep);
  if (idStr.empty()) return false;

  bool ok     = false;
  const int id = QString::fromStdWString(idStr).toInt(&ok);
  if (!ok || id <= 0) return false;

  relative = (sep == std::wstring::npos) ? std::wstring() : rest.substr(sep + 1);
  treeId   = id;
  return true;
}

//! Builds the mapped path of a child of a SAF directory.
TFilePath safChild(const TFilePath &dir, const std::wstring &name) {
  return dir + TFilePath(name);
}

}  // namespace

//=============================================================================
//  Lifetime
//=============================================================================

void initialize() {
  std::call_once(g_initOnce, []() {
    resolveStorageClass();
    resolveStorageMethods();
    g_available = jniReady();
    if (!g_available) {
      __android_log_print(ANDROID_LOG_ERROR, "OpenToonz",
                          "the Android platform layer is unavailable: the "
                          "application will fall back to the Qt standard "
                          "locations");
      return;
    }
    ensureStuffStaging();
  });
}

bool isAvailable() { return g_available; }

//=============================================================================
//  Application specific storage
//=============================================================================

TFilePath getInternalRoot() {
  if (!g_internalRoot.isEmpty()) return g_internalRoot;

  std::wstring root = callStringMethod(SM_getInternalRoot, std::wstring());
  if (root.empty()) {
    // Fallback: the Qt standard location is the same directory.
    root = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
               .toStdWString();
  }
  g_internalRoot = TFilePath(root);
  return g_internalRoot;
}

TFilePath getStuffRoot() {
  if (g_stuffRoot.isEmpty()) g_stuffRoot = getInternalRoot() + L"stuff";
  return g_stuffRoot;
}

TFilePath getCacheRoot() {
  if (!g_cacheRoot.isEmpty()) return g_cacheRoot;

  std::wstring root = callStringMethod(SM_getCacheRoot, std::wstring());
  if (root.empty())
    root = QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
               .toStdWString();
  g_cacheRoot = TFilePath(root);
  return g_cacheRoot;
}

bool ensureStuffExtracted() {
  if (!jniReady()) return false;

  jmethodID mid = storageMethod(SM_ensureAssetsExtracted);
  if (!mid) return false;

  bool attached = false;
  JNIEnv *env   = attachEnv(attached);
  if (!env) return false;

  const std::wstring target = getStuffRoot().getWideString();
  jstring jtarget           = wstringToJstring(env, target);
  jboolean ok =
      env->CallStaticBooleanMethod(storageClass(), mid, jtarget);
  env->DeleteLocalRef(jtarget);

  if (env->ExceptionCheck()) {
    env->ExceptionDescribe();
    env->ExceptionClear();
    ok = JNI_FALSE;
  }

  detachEnv(env, attached);
  return ok == JNI_TRUE;
}

//=============================================================================
//  SAF
//=============================================================================

namespace saf {

TFilePath root() { return getInternalRoot() + L"saf"; }

bool isSafPath(const TFilePath &path) {
  if (path.isEmpty()) return false;
  const std::wstring rootStr = root().getWideString();
  const std::wstring pathStr = path.getWideString();
  if (pathStr.size() <= rootStr.size()) return false;
  if (pathStr.compare(0, rootStr.size(), rootStr) != 0) return false;
  const wchar_t next = pathStr[rootStr.size()];
  return next == L'/' || next == L'\\';
}

std::vector<TreeInfo> trees() {
  std::vector<TreeInfo> out;
  if (!jniReady()) return out;

  jmethodID mid = storageMethod(SM_trees);
  if (!mid) return out;

  bool attached = false;
  JNIEnv *env   = attachEnv(attached);
  if (!env) return out;

  jobjectArray result =
      static_cast<jobjectArray>(env->CallStaticObjectMethod(storageClass(), mid));
  if (env->ExceptionCheck()) {
    env->ExceptionDescribe();
    env->ExceptionClear();
    detachEnv(env, attached);
    return out;
  }
  if (!result) {
    detachEnv(env, attached);
    return out;
  }

  const jsize count = env->GetArrayLength(result);
  for (jsize i = 0; i < count; ++i) {
    jstring entry = static_cast<jstring>(env->GetObjectArrayElement(result, i));
    if (!entry) continue;
    const std::wstring line = jstringToWString(env, entry);
    env->DeleteLocalRef(entry);

    // "<id>\u001f<uri>\u001f<title>"
    const size_t sep1 = line.find(L'\x1f');
    if (sep1 == std::wstring::npos) continue;
    const size_t sep2 = line.find(L'\x1f', sep1 + 1);
    if (sep2 == std::wstring::npos) continue;

    bool ok      = false;
    const int id = QString::fromStdWString(line.substr(0, sep1)).toInt(&ok);
    if (!ok) continue;

    TreeInfo info;
    info.id      = id;
    info.treeUri = line.substr(sep1 + 1, sep2 - sep1 - 1);
    info.title   = line.substr(sep2 + 1);
    if (info.title.empty()) info.title = info.treeUri;
    out.push_back(info);
  }

  env->DeleteLocalRef(result);
  detachEnv(env, attached);
  return out;
}

int addTree(const std::wstring &treeUri, const std::wstring &title) {
  if (treeUri.empty() || !jniReady()) return 0;

  jmethodID mid = storageMethod(SM_addTree);
  if (!mid) return 0;

  bool attached = false;
  JNIEnv *env   = attachEnv(attached);
  if (!env) return 0;

  jstring juri   = wstringToJstring(env, treeUri);
  jstring jtitle = wstringToJstring(env, title);
  const jint id  = env->CallStaticIntMethod(storageClass(), mid, juri, jtitle);
  env->DeleteLocalRef(juri);
  env->DeleteLocalRef(jtitle);

  if (env->ExceptionCheck()) {
    env->ExceptionDescribe();
    env->ExceptionClear();
    detachEnv(env, attached);
    return 0;
  }

  detachEnv(env, attached);
  return static_cast<int>(id);
}

void removeTree(int id) {
  if (id <= 0 || !jniReady()) return;

  jmethodID mid = storageMethod(SM_removeTree);
  if (!mid) return;

  bool attached = false;
  JNIEnv *env   = attachEnv(attached);
  if (!env) return;
  env->CallStaticVoidMethod(storageClass(), mid, static_cast<jint>(id));
  if (env->ExceptionCheck()) env->ExceptionClear();
  detachEnv(env, attached);
}

bool listDirectory(const TFilePath &path, std::vector<std::wstring> &dirs,
                   std::vector<std::wstring> &files) {
  if (!isSafPath(path) || !jniReady()) return false;

  jmethodID mid = storageMethod(SM_listDirectory);
  if (!mid) return false;

  bool attached = false;
  JNIEnv *env   = attachEnv(attached);
  if (!env) return false;

  jstring jpath = wstringToJstring(env, path.getWideString());
  jobjectArray result = static_cast<jobjectArray>(
      env->CallStaticObjectMethod(storageClass(), mid, jpath));
  env->DeleteLocalRef(jpath);

  if (env->ExceptionCheck()) {
    env->ExceptionDescribe();
    env->ExceptionClear();
    detachEnv(env, attached);
    return false;
  }
  if (!result) {
    detachEnv(env, attached);
    return true;  // empty directory
  }

  const jsize count = env->GetArrayLength(result);
  for (jsize i = 0; i < count; ++i) {
    jstring entry =
        static_cast<jstring>(env->GetObjectArrayElement(result, i));
    if (!entry) continue;
    const std::wstring name = jstringToWString(env, entry);
    env->DeleteLocalRef(entry);
    if (name.empty()) continue;

    // The Java side marks directories with a trailing separator, which keeps
    // the bridge to a single round trip per listing.
    if (name.back() == L'/')
      dirs.push_back(name.substr(0, name.size() - 1));
    else
      files.push_back(name);
  }

  env->DeleteLocalRef(result);
  detachEnv(env, attached);
  return true;
}

bool exists(const TFilePath &path) {
  if (!isSafPath(path)) return false;
  // The synthetic root and the tree folders are virtual.
  int treeId = 0;
  std::wstring relative;
  if (!splitSafPath(path, treeId, relative)) return false;
  if (relative.empty()) return true;
  return callBooleanMethod(SM_exists, path.getWideString());
}

bool isDirectory(const TFilePath &path) {
  if (!isSafPath(path)) return false;
  int treeId = 0;
  std::wstring relative;
  if (!splitSafPath(path, treeId, relative)) return false;
  if (relative.empty()) return true;
  return callBooleanMethod(SM_isDirectory, path.getWideString());
}

bool createDirectory(const TFilePath &path) {
  if (!isSafPath(path)) return false;
  return callBooleanMethod(SM_createDirectory, path.getWideString());
}

bool remove(const TFilePath &path, bool recursive) {
  if (!isSafPath(path) || !jniReady()) return false;
  jmethodID mid = storageMethod(SM_remove);
  if (!mid) return false;

  bool attached = false;
  JNIEnv *env   = attachEnv(attached);
  if (!env) return false;

  jstring jpath = wstringToJstring(env, path.getWideString());
  jboolean ok   = env->CallStaticBooleanMethod(storageClass(), mid, jpath,
                                               recursive ? JNI_TRUE : JNI_FALSE);
  env->DeleteLocalRef(jpath);

  if (env->ExceptionCheck()) {
    env->ExceptionDescribe();
    env->ExceptionClear();
    ok = JNI_FALSE;
  }

  detachEnv(env, attached);
  return ok == JNI_TRUE;
}

bool rename(const TFilePath &src, const TFilePath &dst) {
  if (!isSafPath(src) || !isSafPath(dst)) return false;
  return callBooleanMethod2(SM_rename, src.getWideString(), dst.getWideString());
}

TFilePath materialize(const TFilePath &safPath) {
  if (!isSafPath(safPath)) return TFilePath();
  const std::wstring local =
      callStringMethod(SM_materialize, safPath.getWideString());
  return local.empty() ? TFilePath() : TFilePath(local);
}

bool store(const TFilePath &safPath, const TFilePath &localFile) {
  if (!isSafPath(safPath)) return false;
  return callBooleanMethod2(SM_store, safPath.getWideString(),
                            localFile.getWideString());
}

bool copyInto(const TFilePath &localSource, const TFilePath &safDirectory) {
  if (!isSafPath(safDirectory)) return false;
  return callBooleanMethod2(SM_copyInto, localSource.getWideString(),
                            safDirectory.getWideString());
}

bool exportToMediaStore(const TFilePath &localFile,
                        const std::wstring &mimeType) {
  return callBooleanMethod2(SM_exportToMediaStore, localFile.getWideString(),
                            mimeType);
}

TFilePath pickFile(const std::wstring &mimeFilter, bool forWriting) {
  if (!jniReady()) return TFilePath();
  jmethodID mid = storageMethod(SM_pickFile);
  if (!mid) return TFilePath();

  bool attached = false;
  JNIEnv *env   = attachEnv(attached);
  if (!env) return TFilePath();

  jstring jfilter = wstringToJstring(env, mimeFilter);
  jobject result  = env->CallStaticObjectMethod(
      storageClass(), mid, jfilter, forWriting ? JNI_TRUE : JNI_FALSE);
  env->DeleteLocalRef(jfilter);

  TFilePath chosen;
  if (env->ExceptionCheck()) {
    env->ExceptionDescribe();
    env->ExceptionClear();
  } else if (result) {
    const std::wstring path = jstringToWString(env, static_cast<jstring>(result));
    env->DeleteLocalRef(result);
    if (!path.empty()) chosen = TFilePath(path);
  }

  detachEnv(env, attached);
  return chosen;
}

TFilePath pickDirectory() {
  if (!jniReady()) return TFilePath();
  jmethodID mid = storageMethod(SM_pickDirectory);
  if (!mid) return TFilePath();

  bool attached = false;
  JNIEnv *env   = attachEnv(attached);
  if (!env) return TFilePath();

  jobject result = env->CallStaticObjectMethod(storageClass(), mid);

  TFilePath chosen;
  if (env->ExceptionCheck()) {
    env->ExceptionDescribe();
    env->ExceptionClear();
  } else if (result) {
    const std::wstring path = jstringToWString(env, static_cast<jstring>(result));
    env->DeleteLocalRef(result);
    if (!path.empty()) chosen = TFilePath(path);
  }

  detachEnv(env, attached);
  return chosen;
}

}  // namespace saf

//=============================================================================
//  Shell and utility services
//=============================================================================

bool openDocument(const TFilePath &path) {
  return callBooleanMethod(SM_openDocument, path.getWideString());
}

bool shareFile(const TFilePath &path, const std::wstring &mimeType) {
  return callBooleanMethod2(SM_shareFile, path.getWideString(), mimeType);
}

void setKeepScreenOn(bool on) {
  if (!jniReady()) return;
  jmethodID mid = storageMethod(SM_keepScreenOn);
  if (!mid) return;

  bool attached = false;
  JNIEnv *env   = attachEnv(attached);
  if (!env) return;
  env->CallStaticVoidMethod(storageClass(), mid, on ? JNI_TRUE : JNI_FALSE);
  if (env->ExceptionCheck()) env->ExceptionClear();
  detachEnv(env, attached);
}

void showToast(const std::wstring &message) {
  if (!jniReady()) return;
  jmethodID mid = storageMethod(SM_toast);
  if (!mid) return;

  bool attached = false;
  JNIEnv *env   = attachEnv(attached);
  if (!env) return;

  jstring jmsg = wstringToJstring(env, message);
  env->CallStaticVoidMethod(storageClass(), mid, jmsg);
  env->DeleteLocalRef(jmsg);
  if (env->ExceptionCheck()) env->ExceptionClear();
  detachEnv(env, attached);
}

void logMessage(const char *tag, const char *message) {
  __android_log_print(ANDROID_LOG_INFO, tag ? tag : "OpenToonz", "%s",
                      message ? message : "");
}

//=============================================================================
//  Device information
//=============================================================================

namespace {

long long callLongMethod(StorageMethodId id) {
  if (!jniReady()) return 0;
  jmethodID mid = storageMethod(id);
  if (!mid) return 0;

  bool attached = false;
  JNIEnv *env   = attachEnv(attached);
  if (!env) return 0;

  const jlong value = env->CallStaticLongMethod(storageClass(), mid);
  if (env->ExceptionCheck()) {
    env->ExceptionClear();
    detachEnv(env, attached);
    return 0;
  }
  detachEnv(env, attached);
  return static_cast<long long>(value);
}

}  // namespace

long long internalStorageSizeKb() { return callLongMethod(SM_internalStorageSizeKb); }
long long internalStorageFreeKb() { return callLongMethod(SM_internalStorageFreeKb); }

bool isLargeScreen() {
  if (!jniReady()) return false;
  jmethodID mid = storageMethod(SM_isLargeScreen);
  if (!mid) return false;

  bool attached = false;
  JNIEnv *env   = attachEnv(attached);
  if (!env) return false;
  const jboolean value = env->CallStaticBooleanMethod(storageClass(), mid);
  if (env->ExceptionCheck()) env->ExceptionClear();
  detachEnv(env, attached);
  return value == JNI_TRUE;
}

void screenSize(int &widthDp, int &heightDp) {
  widthDp  = 0;
  heightDp = 0;
  if (!jniReady()) return;

  jmethodID mid = storageMethod(SM_screenSize);
  if (!mid) return;

  bool attached = false;
  JNIEnv *env   = attachEnv(attached);
  if (!env) return;

  jintArray result =
      static_cast<jintArray>(env->CallStaticObjectMethod(storageClass(), mid));
  if (env->ExceptionCheck()) {
    env->ExceptionClear();
    detachEnv(env, attached);
    return;
  }
  if (result) {
    jint values[2] = {0, 0};
    env->GetIntArrayRegion(result, 0, 2, values);
    widthDp  = values[0];
    heightDp = values[1];
    env->DeleteLocalRef(result);
  }
  detachEnv(env, attached);
}

}  // namespace otandroid

//=============================================================================
//  JNI entry points invoked by the Java side
//=============================================================================

extern "C" JNIEXPORT void JNICALL
Java_io_github_opentoonz_android_AndroidStorage_nativeSetVm(JNIEnv *env,
                                                            jclass) {
  otandroid::registerVmAndActivity(env, nullptr, true);
}

extern "C" JNIEXPORT void JNICALL
Java_io_github_opentoonz_android_AndroidStorage_nativeRegisterActivity(
    JNIEnv *env, jclass, jobject activity) {
  otandroid::registerVmAndActivity(env, activity, false);
}

extern "C" JNIEXPORT void JNICALL
Java_io_github_opentoonz_android_AndroidStorage_nativeInitialize(JNIEnv *,
                                                                jclass) {
  otandroid::initialize();
}

#endif  // __ANDROID__
