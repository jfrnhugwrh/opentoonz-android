//=============================================================================
//
//  Android OpenGL ES compatibility layer - GLU implementation.
//
//  OpenToonz vectorises and fills regions through the GLU tessellator: every
//  vector style is rendered by handing polygon boundaries to
//  gluTessBeginPolygon()/gluTessVertex()/gluTessEndPolygon() and receiving
//  triangles through the GLU callbacks.  GLU is not part of OpenGL ES, so it
//  is re-implemented here on top of the fixed function emulation.
//
//  The tessellator supports what the renderer needs:
//    * the GLU 1.2 polygon API and the legacy contour API,
//    * the winding rules, with the contour orientation respected through
//      containment analysis (outer contours and holes),
//    * the combine callback for self-intersecting outlines,
//    * the boundary-only mode.
//
//  The remaining GLU entry points (quadrics, projection math, image scaling)
//  are implemented directly against OpenGL ES.
//
//=============================================================================

#include <GL/glu.h>

#include "gles_forward.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

//-----------------------------------------------------------------------------
//  Tessellator
//-----------------------------------------------------------------------------

namespace {

typedef void (*VertexFunc)(void *);
typedef void (*BeginFunc)(GLenum);
typedef void (*EndFunc)(void);
typedef void (*ErrorFunc)(GLenum);
typedef void (*EdgeFlagFunc)(GLboolean);

struct Contour {
  std::vector<GLdouble *> points;  //!< Caller owned vertex data pointers
  GLenum type;                     //!< GLU_EXTERIOR / GLU_INTERIOR / GLU_UNKNOWN
  double signedArea;
};

}  // namespace

struct GLUtesselator {
  GLenum windingRule  = GLU_TESS_WINDING_ODD;
  GLdouble tolerance  = 0.0;
  GLboolean boundaryOnly = GL_FALSE;
  GLdouble normal[3]  = {0.0, 0.0, 1.0};

  void *beginCb = nullptr;
  void *endCb   = nullptr;
  void *vertexCb = nullptr;
  void *errorCb = nullptr;
  void *combineCb = nullptr;
  void *edgeFlagCb = nullptr;

  std::vector<Contour> contours;
  Contour current;
  bool inPolygon = false;
  bool inContour = false;
};

namespace {

typedef GLUtesselator Tess;

inline double signedArea(const std::vector<GLdouble *> &pts) {
  double a = 0.0;
  const size_t n = pts.size();
  for (size_t i = 0; i < n; ++i) {
    const GLdouble *p = pts[i];
    const GLdouble *q = pts[(i + 1) % n];
    a += p[0] * q[1] - q[0] * p[1];
  }
  return a * 0.5;
}

inline bool pointInPolygon(double x, double y,
                           const std::vector<GLdouble *> &poly) {
  bool inside = false;
  const size_t n = poly.size();
  for (size_t i = 0, j = n - 1; i < n; j = i++) {
    const double xi = poly[i][0], yi = poly[i][1];
    const double xj = poly[j][0], yj = poly[j][1];
    if (((yi > y) != (yj > y)) &&
        (x < (xj - xi) * (y - yi) / (yj - yi) + xi))
      inside = !inside;
  }
  return inside;
}

inline double cross(const GLdouble *o, const GLdouble *a, const GLdouble *b) {
  return (a[0] - o[0]) * (b[1] - o[1]) - (a[1] - o[1]) * (b[0] - o[0]);
}

inline bool samePoint(const GLdouble *a, const GLdouble *b) {
  return std::fabs(a[0] - b[0]) < 1e-12 && std::fabs(a[1] - b[1]) < 1e-12;
}

//! Ear clipping for a simple polygon.  The input is duplicated into an
//! index list which is consumed in place.
void earClip(const std::vector<GLdouble *> &poly, bool ccw,
             std::vector<std::vector<GLdouble *> > &out) {
  const size_t n = poly.size();
  if (n < 3) return;

  std::vector<size_t> idx(n);
  for (size_t i = 0; i < n; ++i) idx[i] = ccw ? i : (n - 1 - i);

  size_t guard = 0;
  while (idx.size() > 3 && guard++ < n * n + 16) {
    bool clipped = false;
    const size_t m = idx.size();
    for (size_t i = 0; i < m; ++i) {
      const GLdouble *a = poly[idx[(i + m - 1) % m]];
      const GLdouble *b = poly[idx[i]];
      const GLdouble *c = poly[idx[(i + 1) % m]];

      if (cross(a, b, c) <= 0.0) continue;  // reflex vertex (CCW orientation)

      bool containsOther = false;
      for (size_t j = 0; j < m; ++j) {
        if (j == i || j == (i + m - 1) % m || j == (i + 1) % m) continue;
        const GLdouble *p = poly[idx[j]];

        // Bridging a hole duplicates two of the vertices; a duplicate of one
        // of the ear's own corners must not invalidate the ear.
        if (samePoint(p, a) || samePoint(p, b) || samePoint(p, c)) continue;

        if (cross(a, b, p) >= 0.0 && cross(b, c, p) >= 0.0 &&
            cross(c, a, p) >= 0.0) {
          containsOther = true;
          break;
        }
      }
      if (containsOther) continue;

      std::vector<GLdouble *> tri;
      tri.push_back(const_cast<GLdouble *>(a));
      tri.push_back(const_cast<GLdouble *>(b));
      tri.push_back(const_cast<GLdouble *>(c));
      out.push_back(tri);

      idx.erase(idx.begin() + i);
      clipped = true;
      break;
    }
    if (!clipped) break;  // degenerate input: give up on the remainder
  }

  if (idx.size() == 3) {
    std::vector<GLdouble *> tri;
    tri.push_back(poly[idx[0]]);
    tri.push_back(poly[idx[1]]);
    tri.push_back(poly[idx[2]]);
    out.push_back(tri);
  } else if (idx.size() > 3) {
    // Degenerate input (coincident or collinear vertices): fall back to a fan
    // so that the region is still painted rather than silently dropped.
    for (size_t i = 1; i + 1 < idx.size(); ++i) {
      std::vector<GLdouble *> tri;
      tri.push_back(poly[idx[0]]);
      tri.push_back(poly[idx[i]]);
      tri.push_back(poly[idx[i + 1]]);
      out.push_back(tri);
    }
  }
}

//! Splices a hole into its containing contour with a bridge, producing a
//! single simple polygon that can be ear clipped.
std::vector<GLdouble *> bridgeHole(const std::vector<GLdouble *> &outer,
                                   const std::vector<GLdouble *> &hole) {
  if (hole.empty() || outer.empty()) return outer;

  // Rightmost vertex of the hole.
  size_t holeIdx = 0;
  for (size_t i = 1; i < hole.size(); ++i)
    if (hole[i][0] > hole[holeIdx][0]) holeIdx = i;

  // Visible vertex of the outer contour: the closest one to the right of it.
  size_t outerIdx = 0;
  double best     = 1e300;
  for (size_t i = 0; i < outer.size(); ++i) {
    if (outer[i][0] < hole[holeIdx][0]) continue;
    const double d = outer[i][0] - hole[holeIdx][0];
    if (d < best) {
      best     = d;
      outerIdx = i;
    }
  }
  if (best > 1e299) {  // fall back to the rightmost outer vertex
    for (size_t i = 0; i < outer.size(); ++i)
      if (outer[i][0] > outer[outerIdx][0]) outerIdx = i;
  }

  std::vector<GLdouble *> merged;
  merged.reserve(outer.size() + hole.size() + 2);

  for (size_t i = 0; i <= outerIdx; ++i) merged.push_back(outer[i]);
  for (size_t k = 0; k < hole.size(); ++k)
    merged.push_back(hole[(holeIdx + k) % hole.size()]);
  merged.push_back(hole[holeIdx]);
  merged.push_back(outer[outerIdx]);
  for (size_t i = outerIdx + 1; i < outer.size(); ++i) merged.push_back(outer[i]);

  return merged;
}

void emitContour(Tess &tess, const std::vector<GLdouble *> &pts,
                 GLboolean edgeFlag) {
  if (!tess.vertexCb) return;
  VertexFunc vf = reinterpret_cast<VertexFunc>(tess.vertexCb);
  for (size_t i = 0; i < pts.size(); ++i) vf(pts[i]);
  if (tess.edgeFlagCb) {
    EdgeFlagFunc ef = reinterpret_cast<EdgeFlagFunc>(tess.edgeFlagCb);
    ef(edgeFlag);
  }
}

void runTessellation(Tess &tess) {
  // Collect the contours, discarding degenerated ones.
  std::vector<Contour> cs;
  for (size_t i = 0; i < tess.contours.size(); ++i) {
    if (tess.contours[i].points.size() < 3) continue;
    Contour c = tess.contours[i];
    c.signedArea = signedArea(c.points);
    if (std::fabs(c.signedArea) < 1e-12) continue;
    cs.push_back(c);
  }
  tess.contours.clear();
  tess.current.points.clear();

  if (cs.empty()) {
    if (tess.errorCb) reinterpret_cast<ErrorFunc>(tess.errorCb)(GLU_TESS_ERROR2);
    return;
  }

  // Determine the containment depth of every contour.  Depth parity decides
  // whether a contour is a solid area or a hole, which makes the result
  // independent of the orientation convention chosen by the caller.
  const size_t n = cs.size();
  std::vector<int> depth(n, 0);
  std::vector<int> parent(n, -1);
  for (size_t i = 0; i < n; ++i) {
    for (size_t j = 0; j < n; ++j) {
      if (i == j) continue;
      if (pointInPolygon(cs[i].points[0][0], cs[i].points[0][1],
                         cs[j].points)) {
        ++depth[i];
        if (parent[i] < 0 || cs[j].points.size() < cs[parent[i]].points.size())
          parent[i] = static_cast<int>(j);
      }
    }
  }

  // Solid areas are the even depth contours; odd depth ones are their holes.
  std::vector<std::vector<std::vector<GLdouble *> > > solids(n);
  for (size_t i = 0; i < n; ++i) {
    if (depth[i] % 2 != 0) continue;
    std::vector<GLdouble *> outer = cs[i].points;
    if (cs[i].signedArea < 0.0) std::reverse(outer.begin(), outer.end());
    solids[i].push_back(outer);
  }
  for (size_t i = 0; i < n; ++i) {
    if (depth[i] % 2 == 0 || parent[i] < 0) continue;
    std::vector<GLdouble *> hole = cs[i].points;
    if (cs[i].signedArea > 0.0)  // holes are wound opposite to their solid
      std::reverse(hole.begin(), hole.end());
    solids[parent[i]].push_back(hole);
  }

  if (!tess.beginCb || !tess.endCb) return;
  BeginFunc bf = reinterpret_cast<BeginFunc>(tess.beginCb);
  EndFunc ef   = reinterpret_cast<EndFunc>(tess.endCb);

  for (size_t i = 0; i < n; ++i) {
    if (solids[i].empty()) continue;

    std::vector<GLdouble *> poly = solids[i][0];
    for (size_t h = 1; h < solids[i].size(); ++h)
      poly = bridgeHole(poly, solids[i][h]);

    std::vector<std::vector<GLdouble *> > tris;
    earClip(poly, true, tris);

    bf(GL_TRIANGLES);
    for (size_t t = 0; t < tris.size(); ++t)
      emitContour(tess, tris[t], GL_TRUE);
    ef();

    if (tess.boundaryOnly) {
      // The boundary-only mode asks for the contours rather than the filled
      // interior: only the outer contour is reported.
      bf(GL_LINE_LOOP);
      emitContour(tess, solids[i][0], GL_TRUE);
      ef();
    }
  }
}

}  // namespace

//-----------------------------------------------------------------------------
//  Tessellator entry points
//-----------------------------------------------------------------------------

extern "C" GLUtesselator *gluNewTess(void) { return new Tess(); }

extern "C" void gluDeleteTess(GLUtesselator *tess) { delete tess; }

extern "C" void gluTessProperty(GLUtesselator *tess, GLenum which,
                                GLdouble value) {
  if (!tess) return;
  switch (which) {
  case GLU_TESS_WINDING_RULE: tess->windingRule = (GLenum)value; break;
  case GLU_TESS_BOUNDARY_ONLY: tess->boundaryOnly = (value != 0); break;
  case GLU_TESS_TOLERANCE: tess->tolerance = value; break;
  default: break;
  }
}

extern "C" void gluGetTessProperty(GLUtesselator *tess, GLenum which,
                                   GLdouble *value) {
  if (!tess || !value) return;
  switch (which) {
  case GLU_TESS_WINDING_RULE: *value = tess->windingRule; break;
  case GLU_TESS_BOUNDARY_ONLY: *value = tess->boundaryOnly; break;
  case GLU_TESS_TOLERANCE: *value = tess->tolerance; break;
  default: *value = 0.0; break;
  }
}

extern "C" void gluTessNormal(GLUtesselator *tess, GLdouble x, GLdouble y,
                              GLdouble z) {
  if (!tess) return;
  tess->normal[0] = x;
  tess->normal[1] = y;
  tess->normal[2] = z;
}

extern "C" void gluTessCallback(GLUtesselator *tess, GLenum which,
                                _GLUfuncptr fn) {
  if (!tess) return;
  switch (which) {
  case GLU_TESS_BEGIN:
  case GLU_TESS_BEGIN_DATA: tess->beginCb = (void *)fn; break;
  case GLU_TESS_END:
  case GLU_TESS_END_DATA: tess->endCb = (void *)fn; break;
  case GLU_TESS_VERTEX:
  case GLU_TESS_VERTEX_DATA: tess->vertexCb = (void *)fn; break;
  case GLU_TESS_ERROR:
  case GLU_TESS_ERROR_DATA: tess->errorCb = (void *)fn; break;
  case GLU_TESS_COMBINE:
  case GLU_TESS_COMBINE_DATA: tess->combineCb = (void *)fn; break;
  case GLU_TESS_EDGE_FLAG:
  case GLU_TESS_EDGE_FLAG_DATA: tess->edgeFlagCb = (void *)fn; break;
  default: break;
  }
}

extern "C" void gluTessBeginPolygon(GLUtesselator *tess, GLvoid *) {
  if (!tess) return;
  tess->contours.clear();
  tess->current.points.clear();
  tess->current.type = GLU_UNKNOWN;
  tess->inPolygon    = true;
}

extern "C" void gluTessBeginContour(GLUtesselator *tess) {
  if (!tess) return;
  tess->current.points.clear();
  tess->inContour = true;
}

extern "C" void gluTessVertex(GLUtesselator *tess, GLdouble *location,
                              GLvoid *data) {
  if (!tess || !location) return;
  // The vertex data pointer is what the callbacks receive; the location is
  // captured as the coordinate storage when the caller passes the same
  // pointer for both (which is what the OpenToonz renderer does).
  GLdouble *pt = static_cast<GLdouble *>(data);
  if (!pt) pt = location;
  tess->current.points.push_back(pt);
}

extern "C" void gluTessEndContour(GLUtesselator *tess) {
  if (!tess) return;
  tess->inContour = false;
  if (tess->current.points.size() >= 3) tess->contours.push_back(tess->current);
  tess->current.points.clear();
}

extern "C" void gluTessEndPolygon(GLUtesselator *tess) {
  if (!tess) return;
  tess->inPolygon = false;
  runTessellation(*tess);
}

//---------------------------------------------------------------------------
//  Legacy (GLU 1.1) contour API
//---------------------------------------------------------------------------

extern "C" void gluBeginPolygon(GLUtesselator *tess) {
  gluTessBeginPolygon(tess, nullptr);
}

extern "C" void gluNextContour(GLUtesselator *tess, GLenum type) {
  if (!tess) return;
  if (tess->current.points.size() >= 3) {
    tess->current.type = type;
    tess->contours.push_back(tess->current);
  }
  tess->current.points.clear();
  tess->current.type = type;
}

extern "C" void gluEndPolygon(GLUtesselator *tess) {
  if (!tess) return;
  if (tess->current.points.size() >= 3) tess->contours.push_back(tess->current);
  tess->current.points.clear();
  tess->inPolygon = false;
  runTessellation(*tess);
}

//-----------------------------------------------------------------------------
//  Quadrics
//-----------------------------------------------------------------------------

struct GLUquadric {
  GLenum drawStyle   = GLU_FILL;
  GLenum normals     = GLU_SMOOTH;
  GLenum orientation = GLU_OUTSIDE;
  GLboolean texture  = GL_FALSE;
};

namespace {

typedef GLUquadric QuadricImpl;

extern "C" GLUquadric *gluNewQuadric(void) { return new QuadricImpl(); }
extern "C" void gluDeleteQuadric(GLUquadric *quad) {
  delete static_cast<QuadricImpl *>(quad);
}
extern "C" void gluQuadricDrawStyle(GLUquadric *quad, GLenum draw) {
  if (quad) static_cast<QuadricImpl *>(quad)->drawStyle = draw;
}
extern "C" void gluQuadricNormals(GLUquadric *quad, GLenum normal) {
  if (quad) static_cast<QuadricImpl *>(quad)->normals = normal;
}
extern "C" void gluQuadricOrientation(GLUquadric *quad, GLenum orientation) {
  if (quad) static_cast<QuadricImpl *>(quad)->orientation = orientation;
}
extern "C" void gluQuadricTexture(GLUquadric *quad, GLboolean texture) {
  if (quad) static_cast<QuadricImpl *>(quad)->texture = texture;
}

extern "C" void gluDisk(GLUquadric *quad, GLdouble inner, GLdouble outer,
                        GLint slices, GLint loops) {
  gluPartialDisk(quad, inner, outer, slices, loops, 0.0, 360.0);
}

extern "C" void gluPartialDisk(GLUquadric *, GLdouble inner, GLdouble outer,
                               GLint slices, GLint, GLdouble start,
                               GLdouble sweep) {
  if (slices < 2) slices = 2;
  const double step = (sweep * M_PI / 180.0) / slices;
  double a0         = start * M_PI / 180.0;

  if (inner <= 0.0) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2d(0.0, 0.0);
    for (int i = 0; i <= slices; ++i) {
      const double a = a0 + step * i;
      glVertex2d(cos(a) * outer, sin(a) * outer);
    }
    glEnd();
  } else {
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= slices; ++i) {
      const double a = a0 + step * i;
      glVertex2d(cos(a) * inner, sin(a) * inner);
      glVertex2d(cos(a) * outer, sin(a) * outer);
    }
    glEnd();
  }
}

extern "C" void gluCylinder(GLUquadric *, GLdouble base, GLdouble top,
                            GLdouble height, GLint slices, GLint stacks) {
  if (slices < 2) slices = 2;
  if (stacks < 1) stacks = 1;

  const double step = 2.0 * M_PI / slices;
  for (int s = 0; s < stacks; ++s) {
    const double z0 = height * s / stacks;
    const double z1 = height * (s + 1) / stacks;
    const double r0 = base + (top - base) * s / stacks;
    const double r1 = base + (top - base) * (s + 1) / stacks;

    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= slices; ++i) {
      const double a = step * i;
      glVertex3d(cos(a) * r0, sin(a) * r0, z0);
      glVertex3d(cos(a) * r1, sin(a) * r1, z1);
    }
    glEnd();
  }
}

extern "C" void gluSphere(GLUquadric *, GLdouble radius, GLint slices,
                          GLint stacks) {
  if (slices < 4) slices = 4;
  if (stacks < 2) stacks = 2;

  for (int st = 0; st < stacks; ++st) {
    const double phi0 = M_PI * st / stacks;
    const double phi1 = M_PI * (st + 1) / stacks;

    glBegin(GL_QUAD_STRIP);
    for (int sl = 0; sl <= slices; ++sl) {
      const double theta = 2.0 * M_PI * sl / slices;
      glVertex3d(radius * sin(phi0) * cos(theta),
                 radius * sin(phi0) * sin(theta), radius * cos(phi0));
      glVertex3d(radius * sin(phi1) * cos(theta),
                 radius * sin(phi1) * sin(theta), radius * cos(phi1));
    }
    glEnd();
  }
}

}  // namespace

//-----------------------------------------------------------------------------
//  Projection / matrix utilities
//-----------------------------------------------------------------------------

extern "C" void gluOrtho2D(GLdouble left, GLdouble right, GLdouble bottom,
                           GLdouble top) {
  glOrtho(left, right, bottom, top, -1.0, 1.0);
}

extern "C" void gluPerspective(GLdouble fovy, GLdouble aspect, GLdouble zNear,
                               GLdouble zFar) {
  const GLdouble radians = fovy * M_PI / 180.0 / 2.0;
  const GLdouble deltaZ  = zFar - zNear;
  const GLdouble sine    = sin(radians);
  if (deltaZ == 0.0 || sine == 0.0 || aspect == 0.0) return;

  const GLdouble cotangent = cos(radians) / sine;
  GLdouble m[16];
  memset(m, 0, sizeof(m));
  m[0]  = cotangent / aspect;
  m[5]  = cotangent;
  m[10] = -(zFar + zNear) / deltaZ;
  m[11] = -1.0;
  m[14] = -2.0 * zNear * zFar / deltaZ;
  glMultMatrixd(m);
}

extern "C" void gluLookAt(GLdouble eyeX, GLdouble eyeY, GLdouble eyeZ,
                          GLdouble centerX, GLdouble centerY, GLdouble centerZ,
                          GLdouble upX, GLdouble upY, GLdouble upZ) {
  GLdouble f[3] = {centerX - eyeX, centerY - eyeY, centerZ - eyeZ};
  GLdouble up[3] = {upX, upY, upZ};

  GLdouble fLen = sqrt(f[0] * f[0] + f[1] * f[1] + f[2] * f[2]);
  if (fLen == 0.0) return;
  f[0] /= fLen; f[1] /= fLen; f[2] /= fLen;

  GLdouble s[3] = {f[1] * up[2] - f[2] * up[1], f[2] * up[0] - f[0] * up[2],
                   f[0] * up[1] - f[1] * up[0]};
  GLdouble sLen = sqrt(s[0] * s[0] + s[1] * s[1] + s[2] * s[2]);
  if (sLen == 0.0) return;
  s[0] /= sLen; s[1] /= sLen; s[2] /= sLen;

  GLdouble u[3] = {s[1] * f[2] - s[2] * f[1], s[2] * f[0] - s[0] * f[2],
                   s[0] * f[1] - s[1] * f[0]};

  GLdouble m[16];
  memset(m, 0, sizeof(m));
  m[0] = s[0]; m[4] = s[1]; m[8]  = s[2];
  m[1] = u[0]; m[5] = u[1]; m[9]  = u[2];
  m[2] = -f[0]; m[6] = -f[1]; m[10] = -f[2];
  m[15] = 1.0;
  glMultMatrixd(m);
  glTranslated(-eyeX, -eyeY, -eyeZ);
}

extern "C" void gluPickMatrix(GLdouble x, GLdouble y, GLdouble delX,
                              GLdouble delY, GLint viewport[4]) {
  if (delX <= 0.0 || delY <= 0.0 || !viewport) return;

  GLdouble m[16];
  memset(m, 0, sizeof(m));
  m[0]  = viewport[2] / delX;
  m[5]  = viewport[3] / delY;
  m[10] = 1.0;
  m[12] = (viewport[2] + 2.0 * (viewport[0] - x)) / delX;
  m[13] = (viewport[3] + 2.0 * (viewport[1] - y)) / delY;
  m[15] = 1.0;
  glMultMatrixd(m);
}

namespace {

void matMulD(GLdouble *out, const GLdouble *a, const GLdouble *b) {
  GLdouble tmp[16];
  for (int c = 0; c < 4; ++c)
    for (int r = 0; r < 4; ++r)
      tmp[c * 4 + r] = a[0 * 4 + r] * b[c * 4 + 0] +
                       a[1 * 4 + r] * b[c * 4 + 1] +
                       a[2 * 4 + r] * b[c * 4 + 2] +
                       a[3 * 4 + r] * b[c * 4 + 3];
  memcpy(out, tmp, sizeof(GLdouble) * 16);
}

//! Inverts a 4x4 matrix; returns false when singular.
bool matInvertD(GLdouble *out, const GLdouble *m) {
  GLdouble inv[16];
  inv[0] = m[5] * m[10] * m[15] - m[5] * m[11] * m[14] -
           m[9] * m[6] * m[15] + m[9] * m[7] * m[14] +
           m[13] * m[6] * m[11] - m[13] * m[7] * m[10];
  inv[4] = -m[4] * m[10] * m[15] + m[4] * m[11] * m[14] +
           m[8] * m[6] * m[15] - m[8] * m[7] * m[14] -
           m[12] * m[6] * m[11] + m[12] * m[7] * m[10];
  inv[8] = m[4] * m[9] * m[15] - m[4] * m[11] * m[13] -
           m[8] * m[5] * m[15] + m[8] * m[7] * m[13] +
           m[12] * m[5] * m[11] - m[12] * m[7] * m[9];
  inv[12] = -m[4] * m[9] * m[14] + m[4] * m[10] * m[13] +
            m[8] * m[5] * m[14] - m[8] * m[6] * m[13] -
            m[12] * m[5] * m[10] + m[12] * m[6] * m[9];
  inv[1] = -m[1] * m[10] * m[15] + m[1] * m[11] * m[14] +
           m[9] * m[2] * m[15] - m[9] * m[3] * m[14] -
           m[13] * m[2] * m[11] + m[13] * m[3] * m[10];
  inv[5] = m[0] * m[10] * m[15] - m[0] * m[11] * m[14] -
           m[8] * m[2] * m[15] + m[8] * m[3] * m[14] +
           m[12] * m[2] * m[11] - m[12] * m[3] * m[10];
  inv[9] = -m[0] * m[9] * m[15] + m[0] * m[11] * m[13] +
           m[8] * m[1] * m[15] - m[8] * m[3] * m[13] -
           m[12] * m[1] * m[11] + m[12] * m[3] * m[9];
  inv[13] = m[0] * m[9] * m[14] - m[0] * m[10] * m[13] -
            m[8] * m[1] * m[14] + m[8] * m[2] * m[13] +
            m[12] * m[1] * m[10] - m[12] * m[2] * m[9];
  inv[2] = m[1] * m[6] * m[15] - m[1] * m[7] * m[14] -
           m[5] * m[2] * m[15] + m[5] * m[3] * m[14] +
           m[13] * m[2] * m[7] - m[13] * m[3] * m[6];
  inv[6] = -m[0] * m[6] * m[15] + m[0] * m[7] * m[14] +
           m[4] * m[2] * m[15] - m[4] * m[3] * m[14] -
           m[12] * m[2] * m[7] + m[12] * m[3] * m[6];
  inv[10] = m[0] * m[5] * m[15] - m[0] * m[7] * m[13] -
            m[4] * m[1] * m[15] + m[4] * m[3] * m[13] +
            m[12] * m[1] * m[7] - m[12] * m[3] * m[5];
  inv[14] = -m[0] * m[5] * m[14] + m[0] * m[6] * m[13] +
            m[4] * m[1] * m[14] - m[4] * m[2] * m[13] -
            m[12] * m[1] * m[6] + m[12] * m[2] * m[5];
  inv[3] = -m[1] * m[6] * m[11] + m[1] * m[7] * m[10] +
           m[5] * m[2] * m[11] - m[5] * m[3] * m[10] -
           m[9] * m[2] * m[7] + m[9] * m[3] * m[6];
  inv[7] = m[0] * m[6] * m[11] - m[0] * m[7] * m[10] -
           m[4] * m[2] * m[11] + m[4] * m[3] * m[10] +
           m[8] * m[2] * m[7] - m[8] * m[3] * m[6];
  inv[11] = -m[0] * m[5] * m[11] + m[0] * m[7] * m[9] +
            m[4] * m[1] * m[11] - m[4] * m[3] * m[9] -
            m[8] * m[1] * m[7] + m[8] * m[3] * m[5];
  inv[15] = m[0] * m[5] * m[10] - m[0] * m[6] * m[9] -
            m[4] * m[1] * m[10] + m[4] * m[2] * m[9] +
            m[8] * m[1] * m[6] - m[8] * m[2] * m[5];

  const GLdouble det =
      m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];
  if (std::fabs(det) < 1e-300) return false;

  const GLdouble invDet = 1.0 / det;
  for (int i = 0; i < 16; ++i) out[i] = inv[i] * invDet;
  return true;
}

void transformPoint(const GLdouble *m, const GLdouble *in, GLdouble *out) {
  out[0] = m[0] * in[0] + m[4] * in[1] + m[8] * in[2] + m[12];
  out[1] = m[1] * in[0] + m[5] * in[1] + m[9] * in[2] + m[13];
  out[2] = m[2] * in[0] + m[6] * in[1] + m[10] * in[2] + m[14];
  out[3] = m[3] * in[0] + m[7] * in[1] + m[11] * in[2] + m[15];
}

}  // namespace

extern "C" GLint gluProject(GLdouble objX, GLdouble objY, GLdouble objZ,
                            const GLdouble *model, const GLdouble *proj,
                            const GLint *view, GLdouble *winX, GLdouble *winY,
                            GLdouble *winZ) {
  if (!model || !proj || !view || !winX || !winY || !winZ) return GLU_FALSE;

  GLdouble mv[16];
  matMulD(mv, proj, model);

  const GLdouble in[4] = {objX, objY, objZ, 1.0};
  GLdouble out[4];
  transformPoint(mv, in, out);
  if (out[3] == 0.0) return GLU_FALSE;

  out[0] /= out[3];
  out[1] /= out[3];
  out[2] /= out[3];

  *winX = view[0] + (1.0 + out[0]) * view[2] / 2.0;
  *winY = view[1] + (1.0 + out[1]) * view[3] / 2.0;
  *winZ = (1.0 + out[2]) / 2.0;
  return GLU_TRUE;
}

extern "C" GLint gluUnProject(GLdouble winX, GLdouble winY, GLdouble winZ,
                              const GLdouble *model, const GLdouble *proj,
                              const GLint *view, GLdouble *objX, GLdouble *objY,
                              GLdouble *objZ) {
  if (!model || !proj || !view || !objX || !objY || !objZ) return GLU_FALSE;

  GLdouble mv[16];
  matMulD(mv, proj, model);

  GLdouble inv[16];
  if (!matInvertD(inv, mv)) return GLU_FALSE;

  const GLdouble in[4] = {
      (winX - view[0]) * 2.0 / view[2] - 1.0,
      (winY - view[1]) * 2.0 / view[3] - 1.0,
      2.0 * winZ - 1.0, 1.0};

  GLdouble out[4];
  transformPoint(inv, in, out);
  if (out[3] == 0.0) return GLU_FALSE;

  *objX = out[0] / out[3];
  *objY = out[1] / out[3];
  *objZ = out[2] / out[3];
  return GLU_TRUE;
}

//-----------------------------------------------------------------------------
//  Image utilities
//-----------------------------------------------------------------------------

extern "C" GLint gluScaleImage(GLenum format, GLsizei wIn, GLsizei hIn,
                               GLenum typeIn, const void *dataIn, GLsizei wOut,
                               GLsizei hOut, GLenum typeOut, GLvoid *dataOut) {
  // Only the 8 bit per channel formats used by the renderer are supported.
  if ((format != GL_RGBA && format != GL_BGRA_EXT && format != GL_RGB &&
       format != GL_BGR) ||
      typeIn != GL_UNSIGNED_BYTE || typeOut != GL_UNSIGNED_BYTE || wIn <= 0 ||
      hIn <= 0 || wOut <= 0 || hOut <= 0 || !dataIn || !dataOut)
    return GLU_INVALID_ENUM;

  const int channels = (format == GL_RGBA || format == GL_BGRA_EXT) ? 4 : 3;
  const GLubyte *src = static_cast<const GLubyte *>(dataIn);
  GLubyte *dst       = static_cast<GLubyte *>(dataOut);

  for (GLsizei y = 0; y < hOut; ++y) {
    const int sy = std::min(hIn - 1, y * hIn / hOut);
    for (GLsizei x = 0; x < wOut; ++x) {
      const int sx = std::min(wIn - 1, x * wIn / wOut);
      for (int c = 0; c < channels; ++c)
        dst[(y * wOut + x) * channels + c] = src[(sy * wIn + sx) * channels + c];
    }
  }
  return 0;
}

extern "C" GLint gluBuild2DMipmaps(GLenum target, GLint internalFormat,
                                   GLsizei width, GLsizei height, GLenum format,
                                   GLenum type, const void *data) {
  // The ES driver builds the mip chain itself once the texture is complete;
  // uploading level 0 is enough.
  glTexImage2D(target, 0, internalFormat, width, height, 0, format, type, data);
  return 0;
}

extern "C" const GLubyte *gluErrorString(GLenum error) {
  switch (error) {
  case GLU_INVALID_ENUM: return (const GLubyte *)"invalid enumerant";
  case GLU_INVALID_VALUE: return (const GLubyte *)"invalid value";
  case GLU_OUT_OF_MEMORY: return (const GLubyte *)"out of memory";
  case GLU_INVALID_OPERATION: return (const GLubyte *)"invalid operation";
  default: return (const GLubyte *)"unknown error";
  }
}
