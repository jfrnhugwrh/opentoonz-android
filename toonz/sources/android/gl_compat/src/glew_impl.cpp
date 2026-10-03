//=============================================================================
//
//  Android OpenGL ES compatibility layer - GLEW shim.
//
//  OpenToonz enables optional OpenGL features by testing GLEW capability
//  macros (GLEW_VERSION_3_2, GLEW_EXT_convolution, GLEW_EXT_histogram, ...).
//  GLEW itself is a desktop loader and does not exist for OpenGL ES, so the
//  handful of entry points the application uses are provided here on top of
//  the ES driver.
//
//  The feature macros are supplied by <GL/glew.h> in this directory.
//
//=============================================================================

#include <GL/glew.h>

#include "gles_forward.h"

#include <cstring>

GLboolean glewExperimental = GL_FALSE;

extern "C" {

GLenum glewInit(void) {
  // Nothing to load: on Android the ES entry points are resolved through
  // otgl::resolveGL() and, for the fixed function pipeline, provided by the
  // compatibility layer itself.
  return GLEW_OK;
}

GLboolean glewIsSupported(const char *name) {
  if (!name) return GL_FALSE;
  const GLubyte *ext = glGetString(GL_EXTENSIONS);
  if (!ext) return GL_FALSE;
  return strstr(reinterpret_cast<const char *>(ext), name) ? GL_TRUE : GL_FALSE;
}

GLboolean glewGetExtension(const char *name) { return glewIsSupported(name); }

const GLubyte *glewGetErrorString(GLenum error) { return gluErrorString(error); }

const GLubyte *glewGetString(GLenum name) { return glGetString(name); }

}  // extern "C"
