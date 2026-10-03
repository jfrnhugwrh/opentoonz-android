#pragma once
#ifndef OPENTOONZ_ANDROID_GLES_FORWARD_H
#define OPENTOONZ_ANDROID_GLES_FORWARD_H

//=============================================================================
//
//  Lazily bound OpenGL ES entry points.
//
//  The compatibility layer re-implements - among others - the fixed function
//  state queries (glGetIntegerv, glGetFloatv, ...) and the enable/disable
//  entry points so that it can track the legacy pipeline state.  Those symbols
//  are also exported by libGLESv3, so the shim cannot simply call them by
//  name: the local definition would shadow the driver implementation.
//
//  Every driver function the shim forwards to is therefore resolved, once, at
//  first use, through the platform loader (see resolveGL()).
//
//=============================================================================

#include <GL/gl.h>

#ifdef __ANDROID__
#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>
#else
#include "gles_core_decls.h"
#endif

namespace otgl {

//! Resolves a GL entry point from the platform driver (libGLESv3 on Android,
//! the test backend on the host).  Returns nullptr when unavailable.
void *resolveGL(const char *name);

typedef void (*PFN_glEnable)(GLenum);
typedef void (*PFN_glDisable)(GLenum);
typedef unsigned char (*PFN_glIsEnabled)(GLenum);
typedef void (*PFN_glGetIntegerv)(GLenum, GLint *);
typedef void (*PFN_glGetFloatv)(GLenum, GLfloat *);
typedef unsigned char (*PFN_glGetError)(void);
typedef const unsigned char *(*PFN_glGetString)(GLenum);
typedef void (*PFN_glGetTexLevelParameteriv)(GLenum, GLint, GLenum, GLint *);
typedef void (*PFN_glPixelStorei)(GLenum, GLint);
typedef void (*PFN_glActiveTexture)(GLenum);
typedef void (*PFN_glBlendFunc)(GLenum, GLenum);
typedef void (*PFN_glLineWidth)(GLfloat);
typedef void (*PFN_glReadPixels)(GLint, GLint, GLsizei, GLsizei, GLenum,
                                 GLenum, void *);
typedef void (*PFN_glTexParameteri)(GLenum, GLenum, GLint);
typedef void (*PFN_glGenTextures)(GLsizei, GLuint *);
typedef void (*PFN_glDeleteTextures)(GLsizei, const GLuint *);
typedef void (*PFN_glBindTexture)(GLenum, GLuint);
typedef void (*PFN_glTexImage2D)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint,
                                 GLenum, GLenum, const void *);
typedef void (*PFN_glTexSubImage2D)(GLenum, GLint, GLint, GLint, GLsizei,
                                    GLsizei, GLenum, GLenum, const void *);
typedef void (*PFN_glGenBuffers)(GLsizei, GLuint *);
typedef void (*PFN_glDeleteBuffers)(GLsizei, const GLuint *);
typedef void (*PFN_glBindBuffer)(GLenum, GLuint);
typedef void (*PFN_glBindBufferBase)(GLenum, GLuint, GLuint);
typedef void (*PFN_glBufferData)(GLenum, GLsizeiptr, const void *, GLenum);
typedef void *(*PFN_glMapBufferRange)(GLenum, GLintptr, GLsizeiptr, GLbitfield);
typedef unsigned char (*PFN_glUnmapBuffer)(GLenum);
typedef void (*PFN_glGenQueries)(GLsizei, GLuint *);
typedef void (*PFN_glDeleteQueries)(GLsizei, const GLuint *);
typedef void (*PFN_glBeginQuery)(GLenum, GLuint);
typedef void (*PFN_glEndQuery)(GLenum);
typedef void (*PFN_glGetQueryObjectuiv)(GLuint, GLenum, GLuint *);
typedef void (*PFN_glBeginTransformFeedback)(GLenum);
typedef void (*PFN_glEndTransformFeedback)(void);
typedef void (*PFN_glTransformFeedbackVaryings)(GLuint, GLsizei,
                                                const char *const *, GLenum);
typedef void (*PFN_glRasterizerDiscard)(GLenum);
typedef void (*PFN_glDrawArrays)(GLenum, GLint, GLsizei);

//! Binds a driver entry point on first use.  Call sites use it as
//! OTGL_BIND(glGetIntegerv, PFN_glGetIntegerv) and receive a plain function
//! pointer resolved exactly once.
#define OTGL_BIND(name, type)                                          \
  ([]() -> type {                                                      \
    static type fnPtr = nullptr;                                       \
    if (!fnPtr) fnPtr = reinterpret_cast<type>(otgl::resolveGL(#name)); \
    return fnPtr;                                                      \
  }())

}  // namespace otgl

#endif  // OPENTOONZ_ANDROID_GLES_FORWARD_H
