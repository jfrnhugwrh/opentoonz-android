//=============================================================================
//
//  Host test backend for the OpenGL ES compatibility layer.
//
//  This file is *not* part of the Android build.  It provides just enough of
//  the OpenGL ES 3.0 entry points - plus the otgl::resolveGL() loader - for the
//  compatibility layer to be compiled and linked on a normal desktop machine.
//  That makes it possible to check the shim in a CI job without an Android
//  device or emulator: every code path that only depends on the legacy API can
//  at least be exercised for compilation and linkage.
//
//  Entry points that the compatibility layer re-implements are provided here
//  under a distinct "mock_" name, so that the shim can forward to them without
//  recursing into its own definitions.
//
//=============================================================================

#include <GL/gl.h>

#include "gles_core_decls.h"
#include "gles_forward.h"

#include <cstring>
#include <map>
#include <string>

namespace {

struct MockState {
  GLuint nextTexture    = 1;
  GLuint nextBuffer     = 1;
  GLuint nextQuery      = 1;
  GLuint nextShader     = 1;
  GLuint nextProgram    = 1;
  GLuint boundTexture   = 0;
  GLint maxTextureSize  = 8192;
  GLint viewport[4]     = {0, 0, 1, 1};
  std::map<std::string, GLint> locations;
  std::map<std::string, GLint> attribs;
};

MockState &mock() {
  static MockState s;
  return s;
}

}  // namespace

//-----------------------------------------------------------------------------
//  Driver-side implementations of the overridden entry points
//-----------------------------------------------------------------------------

namespace {

void mock_glEnable(GLenum) {}
void mock_glDisable(GLenum) {}
GLboolean mock_glIsEnabled(GLenum) { return GL_FALSE; }
GLenum mock_glGetError(void) { return GL_NO_ERROR; }
void mock_glPixelStorei(GLenum, GLint) {}
void mock_glActiveTexture(GLenum) {}
void mock_glBlendFunc(GLenum, GLenum) {}
void mock_glLineWidth(GLfloat) {}
void mock_glDrawArrays(GLenum, GLint, GLsizei) {}

void mock_glGetIntegerv(GLenum pname, GLint *params) {
  if (!params) return;
  switch (pname) {
  case GL_MAX_TEXTURE_SIZE: params[0] = mock().maxTextureSize; break;
  case GL_VIEWPORT: memcpy(params, mock().viewport, sizeof(mock().viewport)); break;
  case GL_TEXTURE_BINDING_2D: params[0] = (GLint)mock().boundTexture; break;
  case GL_STENCIL_BITS: params[0] = 8; break;
  default: params[0] = 0; break;
  }
}

void mock_glGetFloatv(GLenum, GLfloat *params) {
  if (params) params[0] = 0.f;
}

void mock_glGetTexLevelParameteriv(GLenum, GLint, GLenum, GLint *params) {
  if (params) params[0] = 0;
}

}  // namespace

//-----------------------------------------------------------------------------
//  Straightforward backend: unused by the shim, present for completeness
//-----------------------------------------------------------------------------

extern "C" {

const GLubyte *glGetString(GLenum) {
  return reinterpret_cast<const GLubyte *>("OpenToonz GL compatibility mock");
}

void glGenTextures(GLsizei n, GLuint *textures) {
  for (GLsizei i = 0; i < n; ++i) textures[i] = mock().nextTexture++;
}
void glDeleteTextures(GLsizei, const GLuint *) {}
void glBindTexture(GLenum, GLuint texture) { mock().boundTexture = texture; }
void glTexParameteri(GLenum, GLenum, GLint) {}
void glTexParameterf(GLenum, GLenum, GLfloat) {}
void glTexImage2D(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum,
                  const void *) {}
void glTexSubImage2D(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum,
                     GLenum, const void *) {}
void glReadPixels(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, void *) {}

void glGenBuffers(GLsizei n, GLuint *buffers) {
  for (GLsizei i = 0; i < n; ++i) buffers[i] = mock().nextBuffer++;
}
void glDeleteBuffers(GLsizei, const GLuint *) {}
void glBindBuffer(GLenum, GLuint) {}
void glBufferData(GLenum, GLsizeiptr, const void *, GLenum) {}
void *glMapBufferRange(GLenum, GLintptr, GLsizeiptr, GLbitfield) {
  return nullptr;
}
GLboolean glUnmapBuffer(GLenum) { return GL_TRUE; }

void glGenQueries(GLsizei n, GLuint *ids) {
  for (GLsizei i = 0; i < n; ++i) ids[i] = mock().nextQuery++;
}
void glDeleteQueries(GLsizei, const GLuint *) {}
void glBeginQuery(GLenum, GLuint) {}
void glEndQuery(GLenum) {}
void glGetQueryObjectuiv(GLuint, GLenum, GLuint *params) {
  if (params) params[0] = 0;
}
void glBeginTransformFeedback(GLenum) {}
void glEndTransformFeedback(void) {}

void glViewport(GLint x, GLint y, GLsizei width, GLsizei height) {
  mock().viewport[0] = x; mock().viewport[1] = y;
  mock().viewport[2] = width; mock().viewport[3] = height;
}

void glEnableVertexAttribArray(GLuint) {}
void glDisableVertexAttribArray(GLuint) {}
void glVertexAttribPointer(GLuint, GLint, GLenum, GLboolean, GLsizei,
                           const void *) {}

GLuint glCreateShader(GLenum) { return mock().nextShader++; }
void glShaderSource(GLuint, GLsizei, const char *const *, const GLint *) {}
void glCompileShader(GLuint) {}
void glGetShaderiv(GLuint, GLenum pname, GLint *params) {
  if (params) params[0] = (pname == GL_COMPILE_STATUS) ? 1 : 0;
}
void glGetShaderInfoLog(GLuint, GLsizei, GLsizei *, char *) {}
void glDeleteShader(GLuint) {}

GLuint glCreateProgram(void) { return mock().nextProgram++; }
void glAttachShader(GLuint, GLuint) {}
void glLinkProgram(GLuint) {}
void glDeleteProgram(GLuint) {}
void glGetProgramiv(GLuint, GLenum pname, GLint *params) {
  if (params) params[0] = (pname == GL_LINK_STATUS) ? 1 : 0;
}
void glGetProgramInfoLog(GLuint, GLsizei, GLsizei *, char *) {}
void glUseProgram(GLuint) {}

GLint glGetAttribLocation(GLuint, const char *name) {
  const std::string key = name ? name : "";
  std::map<std::string, GLint>::iterator it = mock().attribs.find(key);
  if (it != mock().attribs.end()) return it->second;
  const GLint id = static_cast<GLint>(mock().attribs.size());
  mock().attribs[key] = id;
  return id;
}

GLint glGetUniformLocation(GLuint, const char *name) {
  const std::string key = name ? name : "";
  std::map<std::string, GLint>::iterator it = mock().locations.find(key);
  if (it != mock().locations.end()) return it->second;
  const GLint id = static_cast<GLint>(mock().locations.size());
  mock().locations[key] = id;
  return id;
}

void glUniform1i(GLint, GLint) {}
void glUniform1f(GLint, GLfloat) {}
void glUniformMatrix4fv(GLint, GLsizei, GLboolean, const GLfloat *) {}

}  // extern "C"

//-----------------------------------------------------------------------------
//  Host loader
//-----------------------------------------------------------------------------

namespace otgl {

void *resolveGL(const char *name) {
  if (!name) return nullptr;

  struct Entry {
    const char *name;
    void *fn;
  };
  static const Entry kEntries[] = {
      {"glEnable", reinterpret_cast<void *>(&mock_glEnable)},
      {"glDisable", reinterpret_cast<void *>(&mock_glDisable)},
      {"glIsEnabled", reinterpret_cast<void *>(&mock_glIsEnabled)},
      {"glGetIntegerv", reinterpret_cast<void *>(&mock_glGetIntegerv)},
      {"glGetFloatv", reinterpret_cast<void *>(&mock_glGetFloatv)},
      {"glGetError", reinterpret_cast<void *>(&mock_glGetError)},
      {"glGetString", reinterpret_cast<void *>(&glGetString)},
      {"glGetTexLevelParameteriv",
       reinterpret_cast<void *>(&mock_glGetTexLevelParameteriv)},
      {"glPixelStorei", reinterpret_cast<void *>(&mock_glPixelStorei)},
      {"glActiveTexture", reinterpret_cast<void *>(&mock_glActiveTexture)},
      {"glBlendFunc", reinterpret_cast<void *>(&mock_glBlendFunc)},
      {"glLineWidth", reinterpret_cast<void *>(&mock_glLineWidth)},
      {"glGenBuffers", reinterpret_cast<void *>(&glGenBuffers)},
      {"glDeleteBuffers", reinterpret_cast<void *>(&glDeleteBuffers)},
      {"glBindBuffer", reinterpret_cast<void *>(&glBindBuffer)},
      {"glBufferData", reinterpret_cast<void *>(&glBufferData)},
      {"glMapBufferRange", reinterpret_cast<void *>(&glMapBufferRange)},
      {"glUnmapBuffer", reinterpret_cast<void *>(&glUnmapBuffer)},
      {"glDrawArrays", reinterpret_cast<void *>(&mock_glDrawArrays)},
      {"glGetBufferSubData", nullptr},
      {"glGetQueryObjectiv", nullptr},
      {"glGetQueryObjectuiv", reinterpret_cast<void *>(&glGetQueryObjectuiv)},
  };

  for (size_t i = 0; i < sizeof(kEntries) / sizeof(kEntries[0]); ++i) {
    if (strcmp(kEntries[i].name, name) == 0) return kEntries[i].fn;
  }
  return nullptr;
}

}  // namespace otgl
