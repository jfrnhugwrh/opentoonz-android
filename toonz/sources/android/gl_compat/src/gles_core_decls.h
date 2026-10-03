#pragma once
#ifndef OPENTOONZ_ANDROID_GLES_CORE_DECLS_H
#define OPENTOONZ_ANDROID_GLES_CORE_DECLS_H

//=============================================================================
//
//  Host-side declarations of the OpenGL ES 3.0 entry points used by the
//  compatibility layer.
//
//  On Android these come from <GLES3/gl3.h> directly; on a normal desktop the
//  headers are usually absent, so the subset the shim relies upon is declared
//  here and implemented by the test backend (test/mock_gles.cpp).  This keeps
//  the compatibility layer compilable - and therefore verifiable - without an
//  Android toolchain or a device.
//
//  Only included when the build is not targeting Android.
//
//=============================================================================

#ifndef __ANDROID__

#include <GL/gl.h>

typedef char GLcharES;

#ifdef __cplusplus
extern "C" {
#endif

// Buffers ---------------------------------------------------------------------
void glGenBuffers(GLsizei n, GLuint *buffers);
void glDeleteBuffers(GLsizei n, const GLuint *buffers);
void glBindBuffer(GLenum target, GLuint buffer);
void glBufferData(GLenum target, GLsizeiptr size, const void *data,
                  GLenum usage);
void *glMapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length,
                       GLbitfield access);
GLboolean glUnmapBuffer(GLenum target);

// Shaders & programs ----------------------------------------------------------
GLuint glCreateShader(GLenum type);
void glShaderSource(GLuint shader, GLsizei count, const char *const *string,
                    const GLint *length);
void glCompileShader(GLuint shader);
void glGetShaderiv(GLuint shader, GLenum pname, GLint *params);
void glGetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei *length,
                        char *infoLog);
void glDeleteShader(GLuint shader);
GLuint glCreateProgram(void);
void glAttachShader(GLuint program, GLuint shader);
void glLinkProgram(GLuint program);
void glDeleteProgram(GLuint program);
void glGetProgramiv(GLuint program, GLenum pname, GLint *params);
void glGetProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei *length,
                         char *infoLog);
void glUseProgram(GLuint program);
GLint glGetAttribLocation(GLuint program, const char *name);
GLint glGetUniformLocation(GLuint program, const char *name);
void glUniform1i(GLint location, GLint v0);
void glUniform1f(GLint location, GLfloat v0);
void glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose,
                        const GLfloat *value);
void glEnableVertexAttribArray(GLuint index);
void glDisableVertexAttribArray(GLuint index);
void glVertexAttribPointer(GLuint index, GLint size, GLenum type,
                           GLboolean normalized, GLsizei stride,
                           const void *pointer);

// Textures --------------------------------------------------------------------
void glGenTextures(GLsizei n, GLuint *textures);
void glDeleteTextures(GLsizei n, const GLuint *textures);
void glBindTexture(GLenum target, GLuint texture);
void glTexParameteri(GLenum target, GLenum pname, GLint param);
void glTexParameterf(GLenum target, GLenum pname, GLfloat param);
void glTexImage2D(GLenum target, GLint level, GLint internalformat,
                  GLsizei width, GLsizei height, GLint border, GLenum format,
                  GLenum type, const void *pixels);
void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset,
                     GLsizei width, GLsizei height, GLenum format, GLenum type,
                     const void *pixels);
void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format,
                  GLenum type, void *pixels);
void glViewport(GLint x, GLint y, GLsizei width, GLsizei height);

// Queries ---------------------------------------------------------------------
void glGenQueries(GLsizei n, GLuint *ids);
void glDeleteQueries(GLsizei n, const GLuint *ids);
void glBeginQuery(GLenum target, GLuint id);
void glEndQuery(GLenum target);
void glGetQueryObjectuiv(GLuint id, GLenum pname, GLuint *params);
void glBeginTransformFeedback(GLenum primitiveMode);
void glEndTransformFeedback(void);

// Core state (only the subset the shim forwards to) ---------------------------
void glEnable(GLenum cap);
void glDisable(GLenum cap);
GLboolean glIsEnabled(GLenum cap);
void glGetIntegerv(GLenum pname, GLint *data);
void glGetFloatv(GLenum pname, GLfloat *data);
GLenum glGetError(void);
const GLubyte *glGetString(GLenum name);
void glGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname,
                              GLint *params);
void glPixelStorei(GLenum pname, GLint param);
void glActiveTexture(GLenum texture);
void glBlendFunc(GLenum sfactor, GLenum dfactor);
void glLineWidth(GLfloat width);

#ifdef __cplusplus
}
#endif

#endif  // !__ANDROID__

#endif  // OPENTOONZ_ANDROID_GLES_CORE_DECLS_H
