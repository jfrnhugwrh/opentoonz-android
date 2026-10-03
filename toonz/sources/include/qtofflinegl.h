#pragma once

#ifndef QTOFFLINEGL_H
#define QTOFFLINEGL_H

#include <memory>

#if defined(ANDROID)
// The Qt4 style GL context classes (QGLFormat, QGLContext, QGLPixelBuffer) are
// not part of the Android builds of Qt: the offscreen contexts are created
// from QOpenGLContext and QOffscreenSurface only.
#include <QOpenGLContext>
#include <QOffscreenSurface>
#include <QOpenGLFramebufferObject>
#else
#include <QtOpenGL>
#include <QGLFormat>
#include <QGLContext>
#include <QGLPixelBuffer>
#include <QOpenGLFramebufferObject>
#endif

#include "tofflinegl.h"

class QtOfflineGL final : public TOfflineGL::Imp {
public:
  std::shared_ptr<QOpenGLContext> m_context;
  std::shared_ptr<QOpenGLContext> m_oldContext;
  std::shared_ptr<QOffscreenSurface> m_surface;
  std::shared_ptr<QOpenGLFramebufferObject> m_fbo;

  QtOfflineGL(TDimension rasterSize, std::shared_ptr<TOfflineGL::Imp> shared);
  ~QtOfflineGL();

  void createContext(TDimension rasterSize,
                     std::shared_ptr<TOfflineGL::Imp> shared) override;
  void makeCurrent() override;
  void doneCurrent() override;

  void saveCurrentContext();
  void restoreCurrentContext();

  void getRaster(TRaster32P raster) override;
};

//-----------------------------------------------------------------------------

#if !defined(ANDROID)
// PBuffer based offscreen rendering does not exist in OpenGL ES; the
// framebuffer object implementation above replaces it entirely.
class QtOfflineGLPBuffer final : public TOfflineGL::Imp {
public:
  std::shared_ptr<QGLPixelBuffer> m_context;

  QtOfflineGLPBuffer(TDimension rasterSize);
  ~QtOfflineGLPBuffer();

  void createContext(TDimension rasterSize);
  void makeCurrent() override;
  void doneCurrent() override;

  void getRaster(TRaster32P raster) override;
};
#endif  // !ANDROID

#endif
