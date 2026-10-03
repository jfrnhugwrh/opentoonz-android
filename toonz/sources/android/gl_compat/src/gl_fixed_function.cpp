//=============================================================================
//
//  Android OpenGL ES compatibility layer - fixed function emulation.
//
//  OpenToonz - and, more importantly, the TnzCore rendering core inherited
//  from Toonz 4.x - draws through the desktop OpenGL fixed function pipeline:
//  immediate mode (glBegin/glEnd), the matrix stacks, display lists, the
//  imaging subset and GLU tessellation.  None of that exists in OpenGL ES.
//
//  This translation unit provides an implementation of those entry points on
//  top of OpenGL ES 3.0 so that the rendering core can stay *unchanged*:
//  every drawing operation is recorded, transformed through the shadow matrix
//  stacks, then submitted as batched vertex arrays to a small shader program
//  reproducing the fixed function behaviour the core relies on (colours,
//  modulate/replace/decal texture environments, alpha testing, line and point
//  primitives).
//
//  This file deliberately re-defines symbols that also exist in libGLESv3.
//  On ELF platforms a definition in the application's own objects takes
//  precedence over the one in the shared library, so wrapping them here gives
//  the shim full control of the legacy state without breaking the driver.
//
//=============================================================================

#include <GL/gl.h>

#include "gles_forward.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <map>
#include <vector>

#ifdef __ANDROID__
#include <android/log.h>
#define OT_TRACE(...) \
  __android_log_print(ANDROID_LOG_DEBUG, "OpenToonzGL", __VA_ARGS__)
#else
#define OT_TRACE(...) \
  do {                \
  } while (0)
#endif

//-----------------------------------------------------------------------------
//  Driver entry point binding
//-----------------------------------------------------------------------------

namespace {
inline otgl::PFN_glEnable drvEnable() {
  return OTGL_BIND(glEnable, otgl::PFN_glEnable);
}
inline otgl::PFN_glDisable drvDisable() {
  return OTGL_BIND(glDisable, otgl::PFN_glDisable);
}
inline otgl::PFN_glIsEnabled drvIsEnabled() {
  return OTGL_BIND(glIsEnabled, otgl::PFN_glIsEnabled);
}
inline otgl::PFN_glGetIntegerv drvGetIntegerv() {
  return OTGL_BIND(glGetIntegerv, otgl::PFN_glGetIntegerv);
}
inline otgl::PFN_glGetFloatv drvGetFloatv() {
  return OTGL_BIND(glGetFloatv, otgl::PFN_glGetFloatv);
}
inline otgl::PFN_glGetError drvGetError() {
  return OTGL_BIND(glGetError, otgl::PFN_glGetError);
}
inline otgl::PFN_glGetTexLevelParameteriv drvGetTexLevelParameteriv() {
  return OTGL_BIND(glGetTexLevelParameteriv, otgl::PFN_glGetTexLevelParameteriv);
}
inline otgl::PFN_glPixelStorei drvPixelStorei() {
  return OTGL_BIND(glPixelStorei, otgl::PFN_glPixelStorei);
}
inline otgl::PFN_glActiveTexture drvActiveTexture() {
  return OTGL_BIND(glActiveTexture, otgl::PFN_glActiveTexture);
}
inline otgl::PFN_glBlendFunc drvBlendFunc() {
  return OTGL_BIND(glBlendFunc, otgl::PFN_glBlendFunc);
}
inline otgl::PFN_glLineWidth drvLineWidth() {
  return OTGL_BIND(glLineWidth, otgl::PFN_glLineWidth);
}
inline otgl::PFN_glDrawArrays drvDrawArrays() {
  return OTGL_BIND(glDrawArrays, otgl::PFN_glDrawArrays);
}
}  // namespace

//-----------------------------------------------------------------------------
//  Local helpers
//-----------------------------------------------------------------------------

namespace {

typedef float Mat4[16];

inline void matIdentity(Mat4 m) {
  m[0] = 1;  m[1] = 0;  m[2] = 0;  m[3] = 0;
  m[4] = 0;  m[5] = 1;  m[6] = 0;  m[7] = 0;
  m[8] = 0;  m[9] = 0;  m[10] = 1; m[11] = 0;
  m[12] = 0; m[13] = 0; m[14] = 0; m[15] = 1;
}

inline void matMult(Mat4 dst, const Mat4 a, const Mat4 b) {
  Mat4 out;
  for (int c = 0; c < 4; ++c) {
    for (int r = 0; r < 4; ++r) {
      out[c * 4 + r] = a[0 * 4 + r] * b[c * 4 + 0] +
                       a[1 * 4 + r] * b[c * 4 + 1] +
                       a[2 * 4 + r] * b[c * 4 + 2] +
                       a[3 * 4 + r] * b[c * 4 + 3];
    }
  }
  memcpy(dst, out, sizeof(Mat4));
}

struct TextureFormat {
  GLenum format;
  GLenum type;
  GLint internalFormat;
};

inline bool resolveFormat(GLenum format, GLenum type, TextureFormat &out) {
  out.type = type;
  out.format = format;
  out.internalFormat = GL_RGBA8;

  if (format == GL_RGBA || format == GL_BGRA || format == GL_BGRA_EXT) {
    out.internalFormat = GL_RGBA8;
    return true;
  }
  if (format == GL_RGB || format == GL_BGR || format == GL_BGR_EXT) {
    out.internalFormat = GL_RGB8;
    return true;
  }
  if (format == GL_LUMINANCE || format == GL_ALPHA) {
    out.internalFormat = GL_R8;
    return true;
  }
  if (format == GL_LUMINANCE_ALPHA) {
    out.internalFormat = GL_RG8;
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
//  Tracked state
//-----------------------------------------------------------------------------

struct AttribState {
  GLboolean blend;
  GLboolean alphaTest;
  GLboolean texture2D[8];
  GLboolean lighting;
  GLboolean depthTest;
  GLboolean stencilTest;
  GLboolean scissorTest;
  GLboolean cullFace;
  GLboolean lineSmooth;
  GLboolean pointSmooth;
  GLboolean polygonSmooth;
  GLboolean dither;
  GLboolean logicOp;
  GLboolean lineStipple;
  GLboolean normalize;
  GLboolean colorMaterial;

  GLenum blendSrc, blendDst;
  GLenum alphaFunc;
  GLclampf alphaRef;
  GLenum depthFunc;
  GLenum cullMode;
  GLenum logicOpCode;
  GLboolean colorMask[4];
  GLfloat lineWidth;
  GLfloat pointSize;
  GLfloat currentColor[4];
  GLfloat currentNormal[3];
  GLfloat currentTexCoord[8][4];
  GLint unpackAlignment;
  GLint unpackRowLength;
  GLint activeTextureUnit;
  GLenum matrixMode;
  GLint texEnvMode[8];

  AttribState() {
    memset(this, 0, sizeof(*this));
    currentColor[0] = currentColor[1] = currentColor[2] = currentColor[3] = 1.f;
    currentNormal[1] = 0.f;
    currentNormal[2] = 1.f;
    alphaFunc        = GL_ALWAYS;
    alphaRef         = 0.f;
    blendSrc         = GL_ONE;
    blendDst         = GL_ZERO;
    depthFunc        = GL_LESS;
    cullMode         = GL_BACK;
    lineWidth        = 1.f;
    pointSize        = 1.f;
    unpackAlignment  = 4;
    unpackRowLength  = 0;
    matrixMode       = GL_MODELVIEW;
    texEnvMode[0]    = GL_MODULATE;
    colorMask[0] = colorMask[1] = colorMask[2] = colorMask[3] = GL_TRUE;
  }
};

struct MatrixStack {
  std::vector<float *> stack;  // owned 16-float blocks
  MatrixStack() { push(); }
  ~MatrixStack() {
    for (size_t i = 0; i < stack.size(); ++i) delete[] stack[i];
  }
  void push() {
    float *m = new float[16];
    if (stack.empty())
      matIdentity(m);
    else
      memcpy(m, stack.back(), sizeof(float) * 16);
    stack.push_back(m);
  }
  void pop() {
    if (stack.size() <= 1) return;
    delete[] stack.back();
    stack.pop_back();
  }
  float *top() const { return stack.back(); }
};

struct Shader {
  GLuint program     = 0;
  GLint attrPos      = -1;
  GLint attrColor    = -1;
  GLint attrTex      = -1;
  GLint uniMVP       = -1;
  GLint uniUseTex    = -1;
  GLint uniTexEnv    = -1;
  GLint uniAlphaTest = -1;
  GLint uniAlphaRef  = -1;
  GLint uniSampler   = -1;
  bool built         = false;
  bool failed        = false;
};

struct Vertex {
  float x, y, z;
  float r, g, b, a;
  float s, t;
  GLboolean hasTex;
};

struct GLContextState {
  AttribState attr;
  MatrixStack modelView;
  MatrixStack projection;
  MatrixStack texture;
  Shader shader;

  bool inBegin     = false;
  GLenum beginMode = GL_POINTS;
  std::vector<Vertex> vertices;

  bool compiling     = false;
  GLuint compilingId = 0;
  std::map<GLuint, std::vector<unsigned char> > lists;

  GLenum error = GL_NO_ERROR;

  float rasterX = 0.f, rasterY = 0.f;

  GLenum renderMode = GL_RENDER;
  std::vector<GLuint> nameStack;
  std::vector<GLuint> selectNames;
  std::vector<float> selectDepths;

  std::vector<AttribState> attribStack;
};

//! The legacy pipeline state is per OpenGL context; the emulation keeps one
//! instance per thread, which maps 1:1 onto the way TnzCore renders (each
//! rendering thread owns its own offline context).
GLContextState &st() {
  static thread_local GLContextState *s = nullptr;
  if (!s) s = new GLContextState();
  return *s;
}

inline void setError(GLenum e) {
  if (st().error == GL_NO_ERROR) st().error = e;
}

inline int activeUnit() {
  int u = st().attr.activeTextureUnit;
  return (u >= 0 && u < 8) ? u : 0;
}

}  // namespace

//-----------------------------------------------------------------------------
//  Shader reproducing the fixed function pipeline
//-----------------------------------------------------------------------------

namespace {

const char *kVertexShader =
    "#version 300 es\n"
    "in vec3 aPos;\n"
    "in vec4 aColor;\n"
    "in vec2 aTex;\n"
    "uniform mat4 uMVP;\n"
    "out vec4 vColor;\n"
    "out vec2 vTex;\n"
    "void main() {\n"
    "  vColor = aColor;\n"
    "  vTex = aTex;\n"
    "  gl_Position = uMVP * vec4(aPos, 1.0);\n"
    "}\n";

const char *kFragmentShader =
    "#version 300 es\n"
    "precision mediump float;\n"
    "in vec4 vColor;\n"
    "in vec2 vTex;\n"
    "uniform int uUseTex;\n"
    "uniform int uTexEnv;\n"
    "uniform int uAlphaTest;\n"
    "uniform float uAlphaRef;\n"
    "uniform sampler2D uSampler;\n"
    "out vec4 fragColor;\n"
    "void main() {\n"
    "  vec4 c = vColor;\n"
    "  if (uUseTex == 1) {\n"
    "    vec4 t = texture(uSampler, vTex);\n"
    "    if (uTexEnv == 1) c = c * t;\n"          // GL_MODULATE
    "    else if (uTexEnv == 2) c = t;\n"          // GL_REPLACE
    "    else if (uTexEnv == 3) c = vec4(mix(c.rgb, t.rgb, t.a), c.a);\n"  // GL_DECAL
    "    else c = c * t;\n"
    "  }\n"
    "  if (uAlphaTest == 1 && c.a <= uAlphaRef) discard;\n"
    "  fragColor = c;\n"
    "}\n";

GLuint compileStage(GLenum type, const char *src) {
  GLuint sh = glCreateShader(type);
  glShaderSource(sh, 1, &src, nullptr);
  glCompileShader(sh);
  GLint ok = 0;
  glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
  if (!ok) {
    char log[1024] = {0};
    glGetShaderInfoLog(sh, sizeof(log) - 1, nullptr, log);
    OT_TRACE("shader compile failed: %s", log);
    glDeleteShader(sh);
    return 0;
  }
  return sh;
}

void buildShader(Shader &sh) {
  if (sh.built || sh.failed) return;
  sh.built = true;

  GLuint vs = compileStage(GL_VERTEX_SHADER, kVertexShader);
  GLuint fs = compileStage(GL_FRAGMENT_SHADER, kFragmentShader);
  if (!vs || !fs) {
    sh.failed = true;
    if (vs) glDeleteShader(vs);
    if (fs) glDeleteShader(fs);
    return;
  }

  GLuint prog = glCreateProgram();
  glAttachShader(prog, vs);
  glAttachShader(prog, fs);
  glLinkProgram(prog);

  GLint ok = 0;
  glGetProgramiv(prog, GL_LINK_STATUS, &ok);
  glDeleteShader(vs);
  glDeleteShader(fs);
  if (!ok) {
    char log[1024] = {0};
    glGetProgramInfoLog(prog, sizeof(log) - 1, nullptr, log);
    OT_TRACE("program link failed: %s", log);
    glDeleteProgram(prog);
    sh.failed = true;
    return;
  }

  sh.program      = prog;
  sh.attrPos      = glGetAttribLocation(prog, "aPos");
  sh.attrColor    = glGetAttribLocation(prog, "aColor");
  sh.attrTex      = glGetAttribLocation(prog, "aTex");
  sh.uniMVP       = glGetUniformLocation(prog, "uMVP");
  sh.uniUseTex    = glGetUniformLocation(prog, "uUseTex");
  sh.uniTexEnv    = glGetUniformLocation(prog, "uTexEnv");
  sh.uniAlphaTest = glGetUniformLocation(prog, "uAlphaTest");
  sh.uniAlphaRef  = glGetUniformLocation(prog, "uAlphaRef");
  sh.uniSampler   = glGetUniformLocation(prog, "uSampler");
}

//! Emits the recorded vertices.  Quads and polygons are expanded into
//! triangles; everything else maps directly onto an ES primitive.
void submitVertices(const std::vector<Vertex> &verts, GLenum mode) {
  if (verts.size() < 2) return;

  GLContextState &s = st();
  buildShader(s.shader);
  if (s.shader.failed) return;

  std::vector<Vertex> tris;
  GLenum drawMode = GL_TRIANGLES;

  switch (mode) {
  case GL_QUADS: {
    size_t quads = verts.size() / 4;
    tris.reserve(quads * 6);
    for (size_t q = 0; q < quads; ++q) {
      const Vertex *v = &verts[q * 4];
      tris.push_back(v[0]); tris.push_back(v[1]); tris.push_back(v[2]);
      tris.push_back(v[0]); tris.push_back(v[2]); tris.push_back(v[3]);
    }
    break;
  }
  case GL_QUAD_STRIP: {
    for (size_t i = 0; i + 3 < verts.size(); i += 2) {
      tris.push_back(verts[i]);     tris.push_back(verts[i + 1]);
      tris.push_back(verts[i + 3]);
      tris.push_back(verts[i]);     tris.push_back(verts[i + 3]);
      tris.push_back(verts[i + 2]);
    }
    break;
  }
  case GL_POLYGON: {
    // Fan around the first vertex - the vector renderer only ever submits
    // convex, non self intersecting outlines here.
    for (size_t i = 1; i + 1 < verts.size(); ++i) {
      tris.push_back(verts[0]);
      tris.push_back(verts[i]);
      tris.push_back(verts[i + 1]);
    }
    break;
  }
  default:
    drawMode = mode;
    tris     = verts;
    break;
  }

  if (tris.size() < 2) return;

  GLuint boundTex = 0;
  drvGetIntegerv()(GL_TEXTURE_BINDING_2D, reinterpret_cast<GLint *>(&boundTex));

  glUseProgram(s.shader.program);

  // A user supplied MVP: projection * modelview
  float mvp[16];
  matMult(mvp, s.projection.top(), s.modelView.top());
  glUniformMatrix4fv(s.shader.uniMVP, 1, GL_FALSE, mvp);

  glUniform1i(s.shader.uniUseTex, boundTex != 0 && s.attr.texture2D[0]);
  glUniform1i(s.shader.uniTexEnv, s.attr.texEnvMode[activeUnit()]);
  glUniform1i(s.shader.uniAlphaTest,
              (s.attr.alphaTest && s.attr.alphaFunc != GL_ALWAYS) ? 1 : 0);
  glUniform1f(s.shader.uniAlphaRef, s.attr.alphaRef);
  glUniform1i(s.shader.uniSampler, 0);

  drvEnable()(GL_BLEND);
  if (drvBlendFunc()) drvBlendFunc()(s.attr.blendSrc, s.attr.blendDst);

  std::vector<float> pos, col, tex;
  pos.reserve(tris.size() * 3);
  col.reserve(tris.size() * 4);
  tex.reserve(tris.size() * 2);
  for (size_t i = 0; i < tris.size(); ++i) {
    const Vertex &v = tris[i];
    pos.push_back(v.x); pos.push_back(v.y); pos.push_back(v.z);
    col.push_back(v.r); col.push_back(v.g); col.push_back(v.b);
    col.push_back(v.a);
    tex.push_back(v.s); tex.push_back(v.t);
  }

  glEnableVertexAttribArray(s.shader.attrPos);
  glVertexAttribPointer(s.shader.attrPos, 3, GL_FLOAT, GL_FALSE, 0, pos.data());
  glEnableVertexAttribArray(s.shader.attrColor);
  glVertexAttribPointer(s.shader.attrColor, 4, GL_FLOAT, GL_FALSE, 0,
                        col.data());
  glEnableVertexAttribArray(s.shader.attrTex);
  glVertexAttribPointer(s.shader.attrTex, 2, GL_FLOAT, GL_FALSE, 0, tex.data());

  if (drvDrawArrays())
    drvDrawArrays()(drawMode, 0, static_cast<GLsizei>(tris.size()));

  glDisableVertexAttribArray(s.shader.attrPos);
  glDisableVertexAttribArray(s.shader.attrColor);
  glDisableVertexAttribArray(s.shader.attrTex);

  if (!s.attr.blend) drvDisable()(GL_BLEND);
}

}  // namespace

//-----------------------------------------------------------------------------
//  Error / string queries
//-----------------------------------------------------------------------------

extern "C" GLenum glGetError(void) {
  GLContextState &s = st();
  GLenum e          = s.error;
  s.error           = GL_NO_ERROR;
  GLenum hw         = drvGetError()();
  return e != GL_NO_ERROR ? e : hw;
}

//-----------------------------------------------------------------------------
//  Matrix stacks
//-----------------------------------------------------------------------------

namespace {
MatrixStack &currentStack() {
  GLContextState &s = st();
  switch (s.attr.matrixMode) {
  case GL_PROJECTION: return s.projection;
  case GL_TEXTURE: return s.texture;
  case GL_MODELVIEW:
  default: return s.modelView;
  }
}
}  // namespace

extern "C" void glMatrixMode(GLenum mode) { st().attr.matrixMode = mode; }

extern "C" void glLoadIdentity(void) { matIdentity(currentStack().top()); }

extern "C" void glPushMatrix(void) { currentStack().push(); }
extern "C" void glPopMatrix(void) { currentStack().pop(); }

extern "C" void glLoadMatrixf(const GLfloat *v) {
  memcpy(currentStack().top(), v, sizeof(float) * 16);
}

extern "C" void glLoadMatrixd(const GLdouble *v) {
  float *m = currentStack().top();
  for (int i = 0; i < 16; ++i) m[i] = static_cast<float>(v[i]);
}

extern "C" void glMultMatrixf(const GLfloat *v) {
  float tmp[16];
  memcpy(tmp, v, sizeof(tmp));
  MatrixStack &s = currentStack();
  matMult(s.top(), s.top(), tmp);
}

extern "C" void glMultMatrixd(const GLdouble *v) {
  GLfloat f[16];
  for (int i = 0; i < 16; ++i) f[i] = static_cast<GLfloat>(v[i]);
  glMultMatrixf(f);
}

extern "C" void glTranslated(GLdouble x, GLdouble y, GLdouble z) {
  Mat4 t;
  matIdentity(t);
  t[12] = static_cast<float>(x);
  t[13] = static_cast<float>(y);
  t[14] = static_cast<float>(z);
  MatrixStack &s = currentStack();
  matMult(s.top(), s.top(), t);
}

extern "C" void glTranslatef(GLfloat x, GLfloat y, GLfloat z) {
  glTranslated(x, y, z);
}

extern "C" void glScaled(GLdouble x, GLdouble y, GLdouble z) {
  Mat4 t;
  matIdentity(t);
  t[0]  = static_cast<float>(x);
  t[5]  = static_cast<float>(y);
  t[10] = static_cast<float>(z);
  MatrixStack &s = currentStack();
  matMult(s.top(), s.top(), t);
}

extern "C" void glScalef(GLfloat x, GLfloat y, GLfloat z) {
  glScaled(x, y, z);
}

extern "C" void glRotated(GLdouble angle, GLdouble x, GLdouble y, GLdouble z) {
  const double a  = angle * 3.14159265358979323846 / 180.0;
  const double c  = cos(a), sn = sin(a);
  double len = sqrt(x * x + y * y + z * z);
  if (len == 0.0) return;
  x /= len; y /= len; z /= len;

  Mat4 r;
  matIdentity(r);
  r[0]  = static_cast<float>(x * x * (1 - c) + c);
  r[1]  = static_cast<float>(y * x * (1 - c) + z * sn);
  r[2]  = static_cast<float>(x * z * (1 - c) - y * sn);
  r[4]  = static_cast<float>(x * y * (1 - c) - z * sn);
  r[5]  = static_cast<float>(y * y * (1 - c) + c);
  r[6]  = static_cast<float>(y * z * (1 - c) + x * sn);
  r[8]  = static_cast<float>(x * z * (1 - c) + y * sn);
  r[9]  = static_cast<float>(y * z * (1 - c) - x * sn);
  r[10] = static_cast<float>(z * z * (1 - c) + c);

  MatrixStack &s = currentStack();
  matMult(s.top(), s.top(), r);
}

extern "C" void glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z) {
  glRotated(angle, x, y, z);
}

extern "C" void glOrtho(GLdouble l, GLdouble r, GLdouble b, GLdouble t,
                        GLdouble n, GLdouble f) {
  Mat4 m;
  matIdentity(m);
  m[0]  = static_cast<float>(2.0 / (r - l));
  m[5]  = static_cast<float>(2.0 / (t - b));
  m[10] = static_cast<float>(-2.0 / (f - n));
  m[12] = static_cast<float>(-(r + l) / (r - l));
  m[13] = static_cast<float>(-(t + b) / (t - b));
  m[14] = static_cast<float>(-(f + n) / (f - n));
  MatrixStack &s = currentStack();
  matMult(s.top(), s.top(), m);
}

extern "C" void glFrustum(GLdouble l, GLdouble r, GLdouble b, GLdouble t,
                          GLdouble n, GLdouble f) {
  Mat4 m;
  memset(m, 0, sizeof(m));
  m[0]  = static_cast<float>(2.0 * n / (r - l));
  m[5]  = static_cast<float>(2.0 * n / (t - b));
  m[8]  = static_cast<float>((r + l) / (r - l));
  m[9]  = static_cast<float>((t + b) / (t - b));
  m[10] = static_cast<float>(-(f + n) / (f - n));
  m[11] = -1.f;
  m[14] = static_cast<float>(-2.0 * f * n / (f - n));
  MatrixStack &s = currentStack();
  matMult(s.top(), s.top(), m);
}

//-----------------------------------------------------------------------------
//  Current values
//-----------------------------------------------------------------------------

extern "C" void glColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a) {
  GLfloat *c = st().attr.currentColor;
  c[0] = r; c[1] = g; c[2] = b; c[3] = a;
}

extern "C" void glColor4ub(GLubyte r, GLubyte g, GLubyte b, GLubyte a) {
  glColor4f(r / 255.f, g / 255.f, b / 255.f, a / 255.f);
}

extern "C" void glColor4d(GLdouble r, GLdouble g, GLdouble b, GLdouble a) {
  glColor4f(static_cast<GLfloat>(r), static_cast<GLfloat>(g),
            static_cast<GLfloat>(b), static_cast<GLfloat>(a));
}

extern "C" void glColor3f(GLfloat r, GLfloat g, GLfloat b) {
  glColor4f(r, g, b, 1.f);
}
extern "C" void glColor3d(GLdouble r, GLdouble g, GLdouble b) {
  glColor4f(static_cast<GLfloat>(r), static_cast<GLfloat>(g),
            static_cast<GLfloat>(b), 1.f);
}
extern "C" void glColor3ub(GLubyte r, GLubyte g, GLubyte b) {
  glColor4f(r / 255.f, g / 255.f, b / 255.f, 1.f);
}
extern "C" void glColor4fv(const GLfloat *v) { glColor4f(v[0], v[1], v[2], v[3]); }
extern "C" void glColor4dv(const GLdouble *v) {
  glColor4f((GLfloat)v[0], (GLfloat)v[1], (GLfloat)v[2], (GLfloat)v[3]);
}
extern "C" void glColor3fv(const GLfloat *v) { glColor4f(v[0], v[1], v[2], 1.f); }
extern "C" void glColor3dv(const GLdouble *v) {
  glColor4f((GLfloat)v[0], (GLfloat)v[1], (GLfloat)v[2], 1.f);
}
extern "C" void glColor3ubv(const GLubyte *v) { glColor3ub(v[0], v[1], v[2]); }
extern "C" void glColor4ubv(const GLubyte *v) {
  glColor4ub(v[0], v[1], v[2], v[3]);
}

extern "C" void glNormal3f(GLfloat x, GLfloat y, GLfloat z) {
  GLfloat *n = st().attr.currentNormal;
  n[0] = x; n[1] = y; n[2] = z;
}
extern "C" void glNormal3d(GLdouble x, GLdouble y, GLdouble z) {
  glNormal3f((GLfloat)x, (GLfloat)y, (GLfloat)z);
}
extern "C" void glNormal3fv(const GLfloat *v) { glNormal3f(v[0], v[1], v[2]); }
extern "C" void glNormal3dv(const GLdouble *v) {
  glNormal3f((GLfloat)v[0], (GLfloat)v[1], (GLfloat)v[2]);
}

extern "C" void glTexCoord2f(GLfloat s, GLfloat t) {
  GLfloat *v = st().attr.currentTexCoord[activeUnit()];
  v[0] = s; v[1] = t; v[2] = 0.f; v[3] = 1.f;
}
extern "C" void glTexCoord2d(GLdouble s, GLdouble t) {
  glTexCoord2f((GLfloat)s, (GLfloat)t);
}
extern "C" void glTexCoord2fv(const GLfloat *v) { glTexCoord2f(v[0], v[1]); }
extern "C" void glTexCoord2dv(const GLdouble *v) {
  glTexCoord2f((GLfloat)v[0], (GLfloat)v[1]);
}
extern "C" void glTexCoord1f(GLfloat s) { glTexCoord2f(s, 0.f); }
extern "C" void glTexCoord1d(GLdouble s) { glTexCoord2f((GLfloat)s, 0.f); }

//-----------------------------------------------------------------------------
//  Immediate mode
//-----------------------------------------------------------------------------

namespace {
void emitVertex(float x, float y, float z) {
  GLContextState &s = st();
  if (!s.inBegin) {
    setError(GL_INVALID_OPERATION);
    return;
  }
  Vertex v;
  v.x = x; v.y = y; v.z = z;
  v.r = s.attr.currentColor[0];
  v.g = s.attr.currentColor[1];
  v.b = s.attr.currentColor[2];
  v.a = s.attr.currentColor[3];
  const GLfloat *tc = s.attr.currentTexCoord[0];
  v.s      = tc[0];
  v.t      = tc[1];
  v.hasTex = s.attr.texture2D[0];
  s.vertices.push_back(v);
}
}  // namespace

extern "C" void glBegin(GLenum mode) {
  GLContextState &s = st();
  if (s.inBegin) {
    setError(GL_INVALID_OPERATION);
    return;
  }
  s.inBegin   = true;
  s.beginMode = mode;
  s.vertices.clear();
}

extern "C" void glVertex2f(GLfloat x, GLfloat y) { emitVertex(x, y, 0.f); }
extern "C" void glVertex2d(GLdouble x, GLdouble y) {
  emitVertex((float)x, (float)y, 0.f);
}
extern "C" void glVertex2i(GLint x, GLint y) {
  emitVertex((float)x, (float)y, 0.f);
}
extern "C" void glVertex2s(GLshort x, GLshort y) {
  emitVertex((float)x, (float)y, 0.f);
}
extern "C" void glVertex3f(GLfloat x, GLfloat y, GLfloat z) {
  emitVertex(x, y, z);
}
extern "C" void glVertex3d(GLdouble x, GLdouble y, GLdouble z) {
  emitVertex((float)x, (float)y, (float)z);
}
extern "C" void glVertex3i(GLint x, GLint y, GLint z) {
  emitVertex((float)x, (float)y, (float)z);
}
extern "C" void glVertex4f(GLfloat x, GLfloat y, GLfloat z, GLfloat) {
  emitVertex(x, y, z);
}
extern "C" void glVertex4d(GLdouble x, GLdouble y, GLdouble z, GLdouble) {
  emitVertex((float)x, (float)y, (float)z);
}
extern "C" void glVertex2fv(const GLfloat *v) { emitVertex(v[0], v[1], 0.f); }
extern "C" void glVertex2dv(const GLdouble *v) {
  emitVertex((float)v[0], (float)v[1], 0.f);
}
extern "C" void glVertex3fv(const GLfloat *v) { emitVertex(v[0], v[1], v[2]); }
extern "C" void glVertex3dv(const GLdouble *v) {
  emitVertex((float)v[0], (float)v[1], (float)v[2]);
}
extern "C" void glVertex4fv(const GLfloat *v) { emitVertex(v[0], v[1], v[2]); }
extern "C" void glVertex4dv(const GLdouble *v) {
  emitVertex((float)v[0], (float)v[1], (float)v[2]);
}

extern "C" void glEnd(void) {
  GLContextState &s = st();
  if (!s.inBegin) {
    setError(GL_INVALID_OPERATION);
    return;
  }
  s.inBegin = false;

  if (s.compiling) {
    std::vector<unsigned char> &blob = s.lists[s.compilingId];
    size_t offset = blob.size();
    blob.resize(offset + 8 + s.vertices.size() * (9 * 4 + 1));
    unsigned char *p = &blob[offset];
    unsigned int mode = (unsigned int)s.beginMode;
    unsigned int count = (unsigned int)s.vertices.size();
    memcpy(p, &mode, 4); p += 4;
    memcpy(p, &count, 4); p += 4;
    for (size_t i = 0; i < s.vertices.size(); ++i) {
      const Vertex &v = s.vertices[i];
      memcpy(p, &v.x, 4); p += 4;
      memcpy(p, &v.y, 4); p += 4;
      memcpy(p, &v.z, 4); p += 4;
      memcpy(p, &v.r, 4); p += 4;
      memcpy(p, &v.g, 4); p += 4;
      memcpy(p, &v.b, 4); p += 4;
      memcpy(p, &v.a, 4); p += 4;
      memcpy(p, &v.s, 4); p += 4;
      memcpy(p, &v.t, 4); p += 4;
      *p++ = v.hasTex ? 1 : 0;
    }
    s.vertices.clear();
    return;
  }

  submitVertices(s.vertices, s.beginMode);
  s.vertices.clear();
}

//-----------------------------------------------------------------------------
//  Display lists
//-----------------------------------------------------------------------------

extern "C" void glNewList(GLuint list, GLenum) {
  GLContextState &s = st();
  s.compiling       = true;
  s.compilingId     = list;
  s.lists[list].clear();
}

extern "C" void glEndList(void) { st().compiling = false; }

extern "C" void glCallList(GLuint list) {
  GLContextState &s = st();
  std::map<GLuint, std::vector<unsigned char> >::iterator it =
      s.lists.find(list);
  if (it == s.lists.end()) return;

  const unsigned char *p = it->second.data();
  const unsigned char *e = p + it->second.size();

  while (p + 8 <= e) {
    unsigned int mode = 0, count = 0;
    memcpy(&mode, p, 4); p += 4;
    memcpy(&count, p, 4); p += 4;

    std::vector<Vertex> verts;
    verts.resize(count);
    for (unsigned int i = 0; i < count && p + 37 <= e; ++i) {
      Vertex &v = verts[i];
      memcpy(&v.x, p, 4); p += 4;
      memcpy(&v.y, p, 4); p += 4;
      memcpy(&v.z, p, 4); p += 4;
      memcpy(&v.r, p, 4); p += 4;
      memcpy(&v.g, p, 4); p += 4;
      memcpy(&v.b, p, 4); p += 4;
      memcpy(&v.a, p, 4); p += 4;
      memcpy(&v.s, p, 4); p += 4;
      memcpy(&v.t, p, 4); p += 4;
      v.hasTex = (*p++ != 0) ? GL_TRUE : GL_FALSE;
    }
    submitVertices(verts, (GLenum)mode);
  }
}

extern "C" void glCallLists(GLsizei n, GLenum type, const GLvoid *lists) {
  if (!lists || n <= 0) return;
  const GLuint base = st().nameStack.empty() ? 0 : st().nameStack.back();
  for (GLsizei i = 0; i < n; ++i) {
    GLuint name = 0;
    switch (type) {
    case GL_BYTE: name = (GLuint)((const GLbyte *)lists)[i]; break;
    case GL_UNSIGNED_BYTE: name = (GLuint)((const GLubyte *)lists)[i]; break;
    case GL_SHORT: name = (GLuint)((const GLshort *)lists)[i]; break;
    case GL_UNSIGNED_SHORT: name = (GLuint)((const GLushort *)lists)[i]; break;
    default: name = ((const GLuint *)lists)[i]; break;
    }
    glCallList(name + base);
  }
}

extern "C" GLuint glGenLists(GLsizei range) {
  static GLuint next = 1;
  GLuint base       = next;
  next += (GLuint)(range > 0 ? range : 1);
  return base;
}

extern "C" void glDeleteLists(GLuint list, GLsizei range) {
  GLContextState &s = st();
  for (GLsizei i = 0; i < range; ++i) s.lists.erase(list + i);
}

extern "C" void glListBase(GLuint base) { st().nameStack.assign(1, base); }

extern "C" GLboolean glIsList(GLuint list) {
  return st().lists.count(list) ? GL_TRUE : GL_FALSE;
}

//-----------------------------------------------------------------------------
//  State tracking
//-----------------------------------------------------------------------------

extern "C" void glEnable(GLenum cap) {
  GLContextState &s = st();
  switch (cap) {
  case GL_BLEND: s.attr.blend = GL_TRUE; break;
  case GL_ALPHA_TEST: s.attr.alphaTest = GL_TRUE; return;
  case GL_TEXTURE_2D: s.attr.texture2D[activeUnit()] = GL_TRUE; return;
  case GL_LIGHTING: s.attr.lighting = GL_TRUE; return;
  case GL_COLOR_MATERIAL: s.attr.colorMaterial = GL_TRUE; return;
  case GL_NORMALIZE: s.attr.normalize = GL_TRUE; return;
  case GL_LINE_STIPPLE: s.attr.lineStipple = GL_TRUE; return;
  case GL_LOGIC_OP: s.attr.logicOp = GL_TRUE; return;
  case GL_DEPTH_TEST: s.attr.depthTest = GL_TRUE; break;
  case GL_STENCIL_TEST: s.attr.stencilTest = GL_TRUE; break;
  case GL_SCISSOR_TEST: s.attr.scissorTest = GL_TRUE; break;
  case GL_CULL_FACE: s.attr.cullFace = GL_TRUE; break;
  case GL_DITHER: s.attr.dither = GL_TRUE; break;
  case GL_LINE_SMOOTH: s.attr.lineSmooth = GL_TRUE; return;
  case GL_POINT_SMOOTH: s.attr.pointSmooth = GL_TRUE; return;
  case GL_POLYGON_SMOOTH: s.attr.polygonSmooth = GL_TRUE; return;
  default: break;
  }
  if (drvEnable()) drvEnable()(cap);
}

extern "C" void glDisable(GLenum cap) {
  GLContextState &s = st();
  switch (cap) {
  case GL_BLEND: s.attr.blend = GL_FALSE; break;
  case GL_ALPHA_TEST: s.attr.alphaTest = GL_FALSE; return;
  case GL_TEXTURE_2D: s.attr.texture2D[activeUnit()] = GL_FALSE; return;
  case GL_LIGHTING: s.attr.lighting = GL_FALSE; return;
  case GL_COLOR_MATERIAL: s.attr.colorMaterial = GL_FALSE; return;
  case GL_NORMALIZE: s.attr.normalize = GL_FALSE; return;
  case GL_LINE_STIPPLE: s.attr.lineStipple = GL_FALSE; return;
  case GL_LOGIC_OP: s.attr.logicOp = GL_FALSE; return;
  case GL_DEPTH_TEST: s.attr.depthTest = GL_FALSE; break;
  case GL_STENCIL_TEST: s.attr.stencilTest = GL_FALSE; break;
  case GL_SCISSOR_TEST: s.attr.scissorTest = GL_FALSE; break;
  case GL_CULL_FACE: s.attr.cullFace = GL_FALSE; break;
  case GL_DITHER: s.attr.dither = GL_FALSE; break;
  case GL_LINE_SMOOTH: s.attr.lineSmooth = GL_FALSE; return;
  case GL_POINT_SMOOTH: s.attr.pointSmooth = GL_FALSE; return;
  case GL_POLYGON_SMOOTH: s.attr.polygonSmooth = GL_FALSE; return;
  default: break;
  }
  if (drvDisable()) drvDisable()(cap);
}

extern "C" GLboolean glIsEnabled(GLenum cap) {
  GLContextState &s = st();
  switch (cap) {
  case GL_BLEND: return s.attr.blend;
  case GL_ALPHA_TEST: return s.attr.alphaTest;
  case GL_TEXTURE_2D: return s.attr.texture2D[activeUnit()];
  case GL_LIGHTING: return s.attr.lighting;
  case GL_DEPTH_TEST: return s.attr.depthTest;
  case GL_STENCIL_TEST: return s.attr.stencilTest;
  case GL_SCISSOR_TEST: return s.attr.scissorTest;
  case GL_CULL_FACE: return s.attr.cullFace;
  default: break;
  }
  return drvIsEnabled() ? drvIsEnabled()(cap) : GL_FALSE;
}

extern "C" void glBlendFunc(GLenum sfactor, GLenum dfactor) {
  st().attr.blendSrc = sfactor;
  st().attr.blendDst = dfactor;
  if (drvBlendFunc()) drvBlendFunc()(sfactor, dfactor);
}

extern "C" void glAlphaFunc(GLenum func, GLclampf ref) {
  st().attr.alphaFunc = func;
  st().attr.alphaRef  = ref;
}

extern "C" void glPolygonMode(GLenum, GLenum) {}
extern "C" void glLineStipple(GLint, GLushort) {}
extern "C" void glPointSize(GLfloat size) {
  // glPointSize has no ES counterpart; the point size is applied to the
  // recorded geometry instead.
  st().attr.pointSize = size;
}
extern "C" void glLineWidth(GLfloat width) {
  st().attr.lineWidth = width;
  if (drvLineWidth()) drvLineWidth()(width);
}
extern "C" void glLogicOp(GLenum opcode) { st().attr.logicOpCode = opcode; }
extern "C" void glHint(GLenum, GLenum) {}
extern "C" void glDrawBuffer(GLenum) {}
extern "C" void glReadBuffer(GLenum) {}
extern "C" void glPixelZoom(GLfloat, GLfloat) {}
extern "C" void glLightfv(GLenum, GLenum, const GLfloat *) {}
extern "C" void glLightf(GLenum, GLenum, GLfloat) {}
extern "C" void glLightModeli(GLenum, GLint) {}
extern "C" void glLightModelfv(GLenum, const GLfloat *) {}
extern "C" void glMaterialfv(GLenum, GLenum, const GLfloat *) {}
extern "C" void glMaterialf(GLenum, GLenum, GLfloat) {}
extern "C" void glShadeModel(GLenum) {}

extern "C" void glTexEnvf(GLenum target, GLenum pname, GLfloat param) {
  if (target == GL_TEXTURE_ENV && pname == GL_TEXTURE_ENV_MODE)
    st().attr.texEnvMode[activeUnit()] = (GLint)param;
}
extern "C" void glTexEnvi(GLenum target, GLenum pname, GLint param) {
  if (target == GL_TEXTURE_ENV && pname == GL_TEXTURE_ENV_MODE)
    st().attr.texEnvMode[activeUnit()] = param;
}
extern "C" void glTexEnvfv(GLenum, GLenum, const GLfloat *) {}

extern "C" void glActiveTexture(GLenum texture) {
  st().attr.activeTextureUnit =
      (texture >= GL_TEXTURE0 && texture < GL_TEXTURE0 + 8)
          ? (int)(texture - GL_TEXTURE0)
          : 0;
  if (drvActiveTexture()) drvActiveTexture()(texture);
}

extern "C" void glPixelStorei(GLenum pname, GLint param) {
  switch (pname) {
  case GL_UNPACK_ALIGNMENT: st().attr.unpackAlignment = param; break;
  case GL_UNPACK_ROW_LENGTH: st().attr.unpackRowLength = param; break;
  default: break;
  }
  drvPixelStorei()(pname, param);
}

//-----------------------------------------------------------------------------
//  Attribute stack
//-----------------------------------------------------------------------------

extern "C" void glPushAttrib(GLbitfield) {
  st().attribStack.push_back(st().attr);
}

extern "C" void glPopAttrib(void) {
  GLContextState &s = st();
  if (s.attribStack.empty()) {
    setError(GL_STACK_UNDERFLOW);
    return;
  }
  s.attr = s.attribStack.back();
  s.attribStack.pop_back();
}

//-----------------------------------------------------------------------------
//  Pixel transfer
//-----------------------------------------------------------------------------

extern "C" void glRasterPos2d(GLdouble x, GLdouble y) {
  st().rasterX = (float)x;
  st().rasterY = (float)y;
}
extern "C" void glRasterPos2f(GLfloat x, GLfloat y) { glRasterPos2d(x, y); }
extern "C" void glRasterPos2i(GLint x, GLint y) { glRasterPos2d(x, y); }
extern "C" void glRasterPos3f(GLfloat x, GLfloat y, GLfloat) { glRasterPos2d(x, y); }
extern "C" void glRasterPos3d(GLdouble x, GLdouble y, GLdouble) { glRasterPos2d(x, y); }
extern "C" void glRasterPos4f(GLfloat x, GLfloat y, GLfloat, GLfloat) { glRasterPos2d(x, y); }
extern "C" void glRasterPos4fv(const GLfloat *v) { glRasterPos2d(v[0], v[1]); }

extern "C" void glBitmap(GLsizei, GLsizei, GLfloat, GLfloat, GLfloat, GLfloat,
                         const GLubyte *) {}

//! ES has no glDrawPixels; the call is emulated by uploading a scratch texture
//! and drawing a screen aligned textured quad at the current raster position.
extern "C" void glDrawPixels(GLsizei width, GLsizei height, GLenum format,
                             GLenum type, const GLvoid *pixels) {
  TextureFormat fmt;
  if (!resolveFormat(format, type, fmt) || !pixels) {
    setError(GL_INVALID_ENUM);
    return;
  }

  GLuint tex = 0;
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexImage2D(GL_TEXTURE_2D, 0, fmt.internalFormat, width, height, 0,
               fmt.format, fmt.type, pixels);

  const GLboolean prevTex = glIsEnabled(GL_TEXTURE_2D);
  const GLint prevEnv     = st().attr.texEnvMode[activeUnit()];
  glEnable(GL_TEXTURE_2D);
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  const float x = st().rasterX, y = st().rasterY;
  glBegin(GL_QUADS);
  glTexCoord2f(0.f, 0.f); glVertex2f(x, y);
  glTexCoord2f(1.f, 0.f); glVertex2f(x + width, y);
  glTexCoord2f(1.f, 1.f); glVertex2f(x + width, y + height);
  glTexCoord2f(0.f, 1.f); glVertex2f(x, y + height);
  glEnd();

  glPopMatrix();
  if (!prevTex) glDisable(GL_TEXTURE_2D);
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, prevEnv);

  glDeleteTextures(1, &tex);
}

extern "C" void glCopyPixels(GLint, GLint, GLsizei, GLsizei, GLenum) {}

//-----------------------------------------------------------------------------
//  Queries
//-----------------------------------------------------------------------------

extern "C" void glGetDoublev(GLenum pname, GLdouble *params) {
  if (!params) return;
  GLContextState &s = st();

  const float *src = nullptr;
  switch (pname) {
  case GL_MODELVIEW_MATRIX: src = s.modelView.top(); break;
  case GL_PROJECTION_MATRIX: src = s.projection.top(); break;
  case GL_TEXTURE_MATRIX: src = s.texture.top(); break;
  default: break;
  }
  if (src) {
    for (int i = 0; i < 16; ++i) params[i] = (GLdouble)src[i];
    return;
  }

  if (pname == GL_UNPACK_ALIGNMENT) { params[0] = s.attr.unpackAlignment; return; }
  if (pname == GL_UNPACK_ROW_LENGTH) { params[0] = s.attr.unpackRowLength; return; }

  GLfloat f[16] = {0};
  glGetFloatv(pname, f);
  int n = (pname == GL_VIEWPORT) ? 4 : 1;
  for (int i = 0; i < n; ++i) params[i] = (GLdouble)f[i];
}

extern "C" void glGetFloatv(GLenum pname, GLfloat *params) {
  if (!params) return;
  GLContextState &s = st();
  switch (pname) {
  case GL_MODELVIEW_MATRIX: memcpy(params, s.modelView.top(), 16 * sizeof(float)); return;
  case GL_PROJECTION_MATRIX: memcpy(params, s.projection.top(), 16 * sizeof(float)); return;
  case GL_TEXTURE_MATRIX: memcpy(params, s.texture.top(), 16 * sizeof(float)); return;
  case GL_CURRENT_COLOR:
    memcpy(params, s.attr.currentColor, 4 * sizeof(float));
    return;
  case GL_LINE_WIDTH: params[0] = s.attr.lineWidth; return;
  case GL_POINT_SIZE: params[0] = s.attr.pointSize; return;
  case GL_COLOR_WRITEMASK:
    for (int i = 0; i < 4; ++i) params[i] = s.attr.colorMask[i] ? 1.f : 0.f;
    return;
  default: break;
  }
  if (drvGetFloatv()) drvGetFloatv()(pname, params);
}

extern "C" void glGetBooleanv(GLenum pname, GLboolean *params) {
  if (!params) return;
  GLContextState &s = st();
  switch (pname) {
  case GL_COLOR_WRITEMASK:
    memcpy(params, s.attr.colorMask, 4 * sizeof(GLboolean));
    return;
  default: break;
  }
  GLfloat f[16] = {0};
  glGetFloatv(pname, f);
  params[0] = (f[0] != 0.f) ? GL_TRUE : GL_FALSE;
}

extern "C" void glGetIntegerv(GLenum pname, GLint *params) {
  if (!params) return;
  GLContextState &s = st();
  switch (pname) {
  case GL_COLOR_WRITEMASK:
    for (int i = 0; i < 4; ++i) params[i] = s.attr.colorMask[i] ? 1 : 0;
    return;
  case GL_LIST_INDEX:
    params[0] = s.compiling ? (GLint)s.compilingId : 0;
    return;
  case GL_UNPACK_ALIGNMENT: params[0] = s.attr.unpackAlignment; return;
  case GL_UNPACK_ROW_LENGTH: params[0] = s.attr.unpackRowLength; return;
  case GL_STENCIL_BITS: params[0] = 8; return;
  case GL_LINE_WIDTH_RANGE: params[0] = 1; params[1] = 16; return;
  case GL_POINT_SIZE_RANGE: params[0] = 1; params[1] = 64; return;
  case GL_TEXTURE_BINDING_2D: break;
  default: break;
  }
  if (drvGetIntegerv()) drvGetIntegerv()(pname, params);
}

extern "C" void glGetTexLevelParameteriv(GLenum target, GLint level,
                                         GLenum pname, GLint *params) {
  if (!params) return;
  if (target == GL_PROXY_TEXTURE_2D) {
    // Proxy textures are a desktop GL concept.  Report the device maximum for
    // the size queries so the capability probes performed by the texture
    // manager keep working; report 8 bits per channel for the format ones.
    switch (pname) {
    case GL_TEXTURE_WIDTH:
    case GL_TEXTURE_HEIGHT: {
      GLint maxSize = 0;
      glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxSize);
      *params = maxSize;
      return;
    }
    case GL_TEXTURE_INTERNAL_FORMAT: *params = GL_RGBA; return;
    case GL_TEXTURE_RED_SIZE:
    case GL_TEXTURE_GREEN_SIZE:
    case GL_TEXTURE_BLUE_SIZE:
    case GL_TEXTURE_ALPHA_SIZE: *params = 8; return;
    default: *params = 0; return;
    }
  }
  if (drvGetTexLevelParameteriv())
    drvGetTexLevelParameteriv()(target, level, pname, params);
}

//-----------------------------------------------------------------------------
//  Selection & picking (used by the plastic deformer / schematic view)
//-----------------------------------------------------------------------------

extern "C" GLint glRenderMode(GLenum mode) {
  GLContextState &s = st();
  GLint hits        = (GLint)s.selectNames.size();
  s.renderMode      = mode;
  if (mode == GL_SELECT) {
    s.selectNames.clear();
    s.selectDepths.clear();
  }
  return hits;
}

extern "C" void glSelectBuffer(GLsizei, GLuint *) {}

extern "C" void glInitNames(void) { st().nameStack.clear(); }

extern "C" void glLoadName(GLuint name) {
  GLContextState &s = st();
  if (s.nameStack.empty())
    s.nameStack.push_back(name);
  else
    s.nameStack.back() = name;
}

extern "C" void glPushName(GLuint name) { st().nameStack.push_back(name); }

extern "C" void glPopName(void) {
  GLContextState &s = st();
  if (!s.nameStack.empty()) s.nameStack.pop_back();
}

//-----------------------------------------------------------------------------
//  Vertex arrays (client side, legacy)
//-----------------------------------------------------------------------------

namespace {

struct ClientArray {
  GLint size;
  GLenum type;
  GLsizei stride;
  const GLvoid *ptr;
  GLboolean enabled;

  ClientArray() : size(4), type(GL_FLOAT), stride(0), ptr(nullptr),
                  enabled(GL_FALSE) {}

  //! Reads component \p i of element \p index, converting to float.
  float valueAt(GLint index, GLint component) const {
    const GLsizei elemSize = (stride != 0) ? stride : elementSize();
    const char *base = static_cast<const char *>(ptr) + (size_t)index * elemSize;

    switch (type) {
    case GL_DOUBLE:
      return static_cast<float>(
          static_cast<const GLdouble *>(static_cast<const void *>(base))[component]);
    case GL_FLOAT:
      return static_cast<const GLfloat *>(static_cast<const void *>(base))[component];
    case GL_INT:
      return static_cast<float>(
          static_cast<const GLint *>(static_cast<const void *>(base))[component]);
    case GL_SHORT:
      return static_cast<float>(
          static_cast<const GLshort *>(static_cast<const void *>(base))[component]);
    case GL_UNSIGNED_BYTE:
      return static_cast<float>(
                 static_cast<const GLubyte *>(static_cast<const void *>(base))[component]) /
             255.f;
    default:
      return 0.f;
    }
  }

  GLsizei elementSize() const {
    size_t unit = 4;
    switch (type) {
    case GL_DOUBLE: unit = 8; break;
    case GL_FLOAT:
    case GL_INT: unit = 4; break;
    case GL_SHORT: unit = 2; break;
    case GL_UNSIGNED_BYTE: unit = 1; break;
    default: unit = 4; break;
    }
    const GLint n = (size > 0) ? size : 3;
    return static_cast<GLsizei>(unit * n);
  }
};

ClientArray g_clientArrays[8];  //!< vertex, color, texcoord, normal, ...

}  // namespace

extern "C" void glEnableClientState(GLenum cap) {
  switch (cap) {
  case GL_VERTEX_ARRAY: g_clientArrays[0].enabled = GL_TRUE; break;
  case GL_COLOR_ARRAY: g_clientArrays[1].enabled = GL_TRUE; break;
  case GL_TEXTURE_COORD_ARRAY: g_clientArrays[2].enabled = GL_TRUE; break;
  case GL_NORMAL_ARRAY: g_clientArrays[3].enabled = GL_TRUE; break;
  default: break;
  }
}

extern "C" void glDisableClientState(GLenum cap) {
  switch (cap) {
  case GL_VERTEX_ARRAY: g_clientArrays[0].enabled = GL_FALSE; break;
  case GL_COLOR_ARRAY: g_clientArrays[1].enabled = GL_FALSE; break;
  case GL_TEXTURE_COORD_ARRAY: g_clientArrays[2].enabled = GL_FALSE; break;
  case GL_NORMAL_ARRAY: g_clientArrays[3].enabled = GL_FALSE; break;
  default: break;
  }
}

extern "C" void glVertexPointer(GLint size, GLenum type, GLsizei stride,
                                const GLvoid *ptr) {
  g_clientArrays[0].size  = size;
  g_clientArrays[0].type  = type;
  g_clientArrays[0].stride = stride;
  g_clientArrays[0].ptr   = ptr;
}
extern "C" void glColorPointer(GLint size, GLenum type, GLsizei stride,
                               const GLvoid *ptr) {
  g_clientArrays[1].size  = size;
  g_clientArrays[1].type  = type;
  g_clientArrays[1].stride = stride;
  g_clientArrays[1].ptr   = ptr;
}
extern "C" void glTexCoordPointer(GLint size, GLenum type, GLsizei stride,
                                  const GLvoid *ptr) {
  g_clientArrays[2].size  = size;
  g_clientArrays[2].type  = type;
  g_clientArrays[2].stride = stride;
  g_clientArrays[2].ptr   = ptr;
}
extern "C" void glNormalPointer(GLenum type, GLsizei stride, const GLvoid *ptr) {
  g_clientArrays[3].size  = 3;
  g_clientArrays[3].type  = type;
  g_clientArrays[3].stride = stride;
  g_clientArrays[3].ptr   = ptr;
}
extern "C" void glArrayElement(GLint) {}

//! Legacy glDrawArrays: when a client vertex array is enabled the vertices
//! (and any enabled colour/texture coordinate array) are folded into the
//! recorded geometry and submitted through the emulation.  Otherwise the call
//! is passed straight to the driver.
extern "C" void glDrawArrays(GLenum mode, GLint first, GLsizei count) {
  const ClientArray &va = g_clientArrays[0];
  if (!va.enabled || !va.ptr) {
    if (drvDrawArrays()) drvDrawArrays()(mode, first, count);
    return;
  }

  GLContextState &s = st();
  std::vector<Vertex> verts;
  verts.resize(count);

  for (GLsizei i = 0; i < count; ++i) {
    const GLint idx = first + i;
    Vertex &v       = verts[i];
    v.x             = va.valueAt(idx, 0);
    v.y             = va.valueAt(idx, 1);
    v.z             = (va.size >= 3) ? va.valueAt(idx, 2) : 0.f;

    if (g_clientArrays[1].enabled && g_clientArrays[1].ptr) {
      const ClientArray &ca = g_clientArrays[1];
      v.r = ca.valueAt(idx, 0);
      v.g = ca.valueAt(idx, 1);
      v.b = ca.valueAt(idx, 2);
      v.a = (ca.size >= 4) ? ca.valueAt(idx, 3) : 1.f;
    } else {
      v.r = s.attr.currentColor[0];
      v.g = s.attr.currentColor[1];
      v.b = s.attr.currentColor[2];
      v.a = s.attr.currentColor[3];
    }

    if (g_clientArrays[2].enabled && g_clientArrays[2].ptr) {
      v.s = g_clientArrays[2].valueAt(idx, 0);
      v.t = g_clientArrays[2].valueAt(idx, 1);
    } else {
      v.s = s.attr.currentTexCoord[0][0];
      v.t = s.attr.currentTexCoord[0][1];
    }
    v.hasTex = s.attr.texture2D[0];
  }

  submitVertices(verts, mode);
}
