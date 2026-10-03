#pragma once
#ifndef OPENTOONZ_ANDROID_GLEW_H
#define OPENTOONZ_ANDROID_GLEW_H

#include <GL/gl.h>
#include <GL/glu.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GLEW_OK 0
#define GLEW_NO_ERROR 0
#define GLEW_ERROR_NO_GL_CONTEXT 1
#define GLEW_ERROR_GL_VERSION_10_ONLY 2
#define GLEW_ERROR_GLX_VERSION_11_ONLY 3

#define GLEW_VERSION 1
#define GLEW_VERSION_MAJOR 2
#define GLEW_VERSION_MINOR 1
#define GLEW_VERSION_MICRO 0

extern GLboolean glewExperimental;

GLenum glewInit(void);
GLboolean glewIsSupported(const char *name);
GLboolean glewGetExtension(const char *name);
const GLubyte *glewGetErrorString(GLenum error);
const GLubyte *glewGetString(GLenum name);

#ifdef __cplusplus
}
#endif

#endif /* OPENTOONZ_ANDROID_GLEW_H */
