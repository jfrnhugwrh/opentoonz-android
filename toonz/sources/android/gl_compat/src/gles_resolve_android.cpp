//=============================================================================
//
//  Android implementation of otgl::resolveGL().
//
//  Only used on Android, where libGLESv3 is loaded by the platform loader.
//  The library handle is kept open for the lifetime of the process: the GL
//  driver is never unloaded.
//
//=============================================================================

#include "gles_forward.h"

#ifdef __ANDROID__

#include <dlfcn.h>
#include <cstring>

namespace otgl {

namespace {

void *glesHandle() {
  static void *handle = nullptr;
  if (!handle) {
    handle = dlopen("libGLESv3.so", RTLD_NOW | RTLD_LOCAL);
    if (!handle) handle = dlopen("libGLESv2.so", RTLD_NOW | RTLD_LOCAL);
  }
  return handle;
}

}  // namespace

void *resolveGL(const char *name) {
  if (!name) return nullptr;

  // eglGetProcAddress is the canonical way to reach extension entry points,
  // but the core ES entry points are exported directly by the driver library.
  void *handle = glesHandle();
  if (!handle) return nullptr;

  return dlsym(handle, name);
}

}  // namespace otgl

#endif  // __ANDROID__
