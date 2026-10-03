//=============================================================================
//
//  Smoke test for the OpenGL ES compatibility layer.
//
//  Verifies the parts of the shim that carry real algorithmic responsibility:
//  the GLU tessellator (which replaces the desktop GLU implementation the
//  vector renderer depends on) and the projection math.  The test is built and
//  run by the Android CI workflow on the host, so a regression in the shim is
//  caught without an emulator.
//
//=============================================================================

#include <GL/gl.h>
#include <GL/glu.h>

#include <cmath>
#include <cstdio>
#include <vector>

namespace {

struct Vertex {
  double x, y;
};

std::vector<Vertex> g_vertices;
std::vector<GLenum> g_primitiveModes;
int g_beginCalls = 0;
int g_endCalls   = 0;

extern "C" {

void CALLBACK onBegin(GLenum mode) {
  ++g_beginCalls;
  g_primitiveModes.push_back(mode);
}

void CALLBACK onEnd(void) { ++g_endCalls; }

// The renderer passes the vertex' own coordinate storage as the callback data
// (gluTessVertex(tess, &pt.x, &pt.x)), so the callback receives a double[3].
void CALLBACK onVertex(void *data) {
  const GLdouble *p = static_cast<const GLdouble *>(data);
  Vertex v = {p[0], p[1]};
  g_vertices.push_back(v);
}

}  // extern "C"

double triangleArea(const Vertex &a, const Vertex &b, const Vertex &c) {
  return std::fabs((b.x - a.x) * (c.y - a.y) - (c.x - a.x) * (b.y - a.y)) * 0.5;
}

int failures = 0;

void check(bool condition, const char *what) {
  std::printf("%s %s\n", condition ? "[ ok ]" : "[FAIL]", what);
  if (!condition) ++failures;
}

void resetCallbacks() {
  g_vertices.clear();
  g_primitiveModes.clear();
  g_beginCalls = g_endCalls = 0;
}

//! A square with a square hole: the classic case the vector renderer produces
//! for a filled region with an inner contour.
void testSquareWithHole() {
  resetCallbacks();

  GLUtesselator *tess = gluNewTess();
  gluTessCallback(tess, GLU_TESS_BEGIN, reinterpret_cast<_GLUfuncptr>(onBegin));
  gluTessCallback(tess, GLU_TESS_END, reinterpret_cast<_GLUfuncptr>(onEnd));
  gluTessCallback(tess, GLU_TESS_VERTEX,
                  reinterpret_cast<_GLUfuncptr>(onVertex));

  static GLdouble outer[4][3] = {
      {0.0, 0.0, 0.0}, {10.0, 0.0, 0.0}, {10.0, 10.0, 0.0}, {0.0, 10.0, 0.0}};
  static GLdouble inner[4][3] = {  // hole, wound clockwise
      {3.0, 3.0, 0.0}, {3.0, 6.0, 0.0}, {6.0, 6.0, 0.0}, {6.0, 3.0, 0.0}};

  gluTessBeginPolygon(tess, nullptr);
  gluTessBeginContour(tess);
  for (int i = 0; i < 4; ++i) gluTessVertex(tess, outer[i], outer[i]);
  gluTessEndContour(tess);
  gluTessBeginContour(tess);
  for (int i = 0; i < 4; ++i) gluTessVertex(tess, inner[i], inner[i]);
  gluTessEndContour(tess);
  gluTessEndPolygon(tess);

  gluDeleteTess(tess);

  check(g_beginCalls > 0 && g_beginCalls == g_endCalls,
        "tessellator emits balanced begin/end pairs");

  double area = 0.0;
  bool allTriangles = true;
  for (size_t i = 0; i < g_primitiveModes.size(); ++i)
    if (g_primitiveModes[i] != GL_TRIANGLES) allTriangles = false;
  for (size_t i = 0; i + 2 < g_vertices.size(); i += 3)
    area += triangleArea(g_vertices[i], g_vertices[i + 1], g_vertices[i + 2]);

  check(allTriangles, "tessellator emits triangles only");

  // 10x10 square minus a 3x3 hole.
  const double expected = 100.0 - 9.0;
  check(std::fabs(area - expected) < 1e-6,
        "filled area equals the outer contour minus the hole");
}

//! Self contained concave outline: the ear clipper must still produce a
//! partition of the original area.
void testConcaveOutline() {
  resetCallbacks();

  GLUtesselator *tess = gluNewTess();
  gluTessCallback(tess, GLU_TESS_BEGIN, reinterpret_cast<_GLUfuncptr>(onBegin));
  gluTessCallback(tess, GLU_TESS_END, reinterpret_cast<_GLUfuncptr>(onEnd));
  gluTessCallback(tess, GLU_TESS_VERTEX,
                  reinterpret_cast<_GLUfuncptr>(onVertex));

  // L shaped outline, counter clockwise.
  static GLdouble pts[6][3] = {{0.0, 0.0, 0.0}, {4.0, 0.0, 0.0},
                               {4.0, 2.0, 0.0}, {2.0, 2.0, 0.0},
                               {2.0, 4.0, 0.0}, {0.0, 4.0, 0.0}};

  gluTessBeginPolygon(tess, nullptr);
  gluTessBeginContour(tess);
  for (int i = 0; i < 6; ++i) gluTessVertex(tess, pts[i], pts[i]);
  gluTessEndContour(tess);
  gluTessEndPolygon(tess);
  gluDeleteTess(tess);

  double area = 0.0;
  for (size_t i = 0; i + 2 < g_vertices.size(); i += 3)
    area += triangleArea(g_vertices[i], g_vertices[i + 1], g_vertices[i + 2]);

  check(std::fabs(area - 12.0) < 1e-6, "concave outline is fully tessellated");
}

void testProjection() {
  GLdouble proj[16], model[16];
  for (int i = 0; i < 16; ++i) proj[i] = model[i] = 0.0;
  proj[0] = 1.0; proj[5] = 1.0; proj[10] = 1.0; proj[15] = 1.0;
  model[0] = 1.0; model[5] = 1.0; model[10] = 1.0; model[15] = 1.0;

  GLint viewport[4] = {0, 0, 100, 100};

  GLdouble winX = 0, winY = 0, winZ = 0;
  gluProject(0.0, 0.0, 0.0, model, proj, viewport, &winX, &winY, &winZ);
  check(std::fabs(winX - 50.0) < 1e-9 && std::fabs(winY - 50.0) < 1e-9,
        "gluProject maps the origin to the viewport centre");

  GLdouble objX = 0, objY = 0, objZ = 0;
  gluUnProject(winX, winY, winZ, model, proj, viewport, &objX, &objY, &objZ);
  check(std::fabs(objX) < 1e-9 && std::fabs(objY) < 1e-9,
        "gluUnProject inverts gluProject");

  GLdouble m[16];
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0.0, 640.0, 0.0, 480.0);
  glGetDoublev(GL_PROJECTION_MATRIX, m);
  check(std::fabs(m[0] - 2.0 / 640.0) < 1e-9 && std::fabs(m[12] + 1.0) < 1e-6,
        "gluOrtho2D builds the expected projection matrix");
}

//! Exercises the immediate mode recording path end to end: without a live GL
//! context the shim must still accept the geometry and report no errors.
void testImmediateMode() {
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glColor4ub(255, 128, 0, 255);
  glBegin(GL_QUADS);
  glVertex2d(0.0, 0.0);
  glVertex2d(1.0, 0.0);
  glVertex2d(1.0, 1.0);
  glVertex2d(0.0, 1.0);
  glEnd();
  check(glGetError() == GL_NO_ERROR, "immediate mode recording is error free");

  GLdouble m[16];
  glGetDoublev(GL_MODELVIEW_MATRIX, m);
  check(std::fabs(m[0] - 1.0) < 1e-6 && std::fabs(m[5] - 1.0) < 1e-6,
        "matrix stack mirrors the recorded transformations");

  glPushMatrix();
  glTranslated(10.0, 20.0, 0.0);
  glGetDoublev(GL_MODELVIEW_MATRIX, m);
  check(std::fabs(m[12] - 10.0) < 1e-9 && std::fabs(m[13] - 20.0) < 1e-9,
        "glTranslated is applied to the current matrix");
  glPopMatrix();
  glGetDoublev(GL_MODELVIEW_MATRIX, m);
  check(std::fabs(m[12]) < 1e-6, "glPopMatrix restores the previous matrix");
}

}  // namespace

int main() {
  std::printf("OpenToonz Android GL compatibility layer - smoke test\n");
  std::printf("-----------------------------------------------------\n");

  testSquareWithHole();
  testConcaveOutline();
  testProjection();
  testImmediateMode();

  std::printf("-----------------------------------------------------\n");
  if (failures == 0) {
    std::printf("all checks passed\n");
    return 0;
  }
  std::printf("%d check(s) failed\n", failures);
  return 1;
}
