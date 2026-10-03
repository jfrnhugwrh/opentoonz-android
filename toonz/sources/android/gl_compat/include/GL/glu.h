#pragma once
#ifndef OPENTOONZ_ANDROID_GLU_H
#define OPENTOONZ_ANDROID_GLU_H

#include <GL/gl.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef GLAPIENTRY
#define GLAPIENTRY
#endif

/* The desktop GLU headers define CALLBACK for source compatibility with the
   Windows convention; the tessellator callbacks of the renderer are declared
   with it. */
#ifndef CALLBACK
#define CALLBACK
#endif
#ifndef GLUAPIENTRY
#define GLUAPIENTRY
#endif

/* GLU Version */
#define GLU_VERSION_1_1 1
#define GLU_VERSION_1_2 1

/* GLU Boolean */
#define GLU_FALSE 0
#define GLU_TRUE 1

/* GLU Error codes */
#define GLU_INVALID_ENUM 100900
#define GLU_INVALID_VALUE 100901
#define GLU_OUT_OF_MEMORY 100902
#define GLU_INCOMPATIBLE_GL_VERSION 100903
#define GLU_INVALID_OPERATION 100904

/* GLU Quadric constants */
#define GLU_SMOOTH 100000
#define GLU_FLAT 100001
#define GLU_NONE 100002
#define GLU_POINT 100010
#define GLU_LINE 100011
#define GLU_FILL 100012
#define GLU_SILHOUETTE 100013
#define GLU_OUTSIDE 100020
#define GLU_INSIDE 100021

/* GLU Tessellation constants */
#define GLU_TESS_MAX_COORD 1.0e150

#define GLU_TESS_WINDING_RULE 100140
#define GLU_TESS_BOUNDARY_ONLY 100141
#define GLU_TESS_TOLERANCE 100142

#define GLU_TESS_WINDING_ODD 100130
#define GLU_TESS_WINDING_NONZERO 100131
#define GLU_TESS_WINDING_POSITIVE 100132
#define GLU_TESS_WINDING_NEGATIVE 100133
#define GLU_TESS_WINDING_ABS_GEQ_TWO 100134

#define GLU_TESS_BEGIN 100100
#define GLU_BEGIN 100100
#define GLU_TESS_VERTEX 100101
#define GLU_VERTEX 100101
#define GLU_TESS_END 100102
#define GLU_END 100102
#define GLU_TESS_ERROR 100103
#define GLU_TESS_EDGE_FLAG 100104
#define GLU_EDGE_FLAG 100104
#define GLU_TESS_COMBINE 100105
#define GLU_TESS_BEGIN_DATA 100106
#define GLU_TESS_VERTEX_DATA 100107
#define GLU_TESS_END_DATA 100108
#define GLU_TESS_ERROR_DATA 100109
#define GLU_TESS_EDGE_FLAG_DATA 100110
#define GLU_TESS_COMBINE_DATA 100111

#define GLU_CW 100120
#define GLU_CCW 100121
#define GLU_INTERIOR 100122
#define GLU_EXTERIOR 100123
#define GLU_UNKNOWN 100124

#define GLU_TESS_ERROR1 100151
#define GLU_TESS_ERROR2 100152
#define GLU_TESS_ERROR3 100153
#define GLU_TESS_ERROR4 100154
#define GLU_TESS_ERROR5 100155
#define GLU_TESS_ERROR6 100156
#define GLU_TESS_ERROR7 100157
#define GLU_TESS_ERROR8 100158
#define GLU_TESS_MISSING_BEGIN_POLYGON 100151
#define GLU_TESS_MISSING_BEGIN_CONTOUR 100152
#define GLU_TESS_MISSING_END_POLYGON 100153
#define GLU_TESS_MISSING_END_CONTOUR 100154
#define GLU_TESS_COORD_TOO_LARGE 100155
#define GLU_TESS_NEED_COMBINE_CALLBACK 100156

typedef struct GLUquadric GLUquadric;
typedef GLUquadric GLUquadricObj;

typedef struct GLUtesselator GLUtesselator;
typedef GLUtesselator GLUtesselatorObj;
typedef GLUtesselator GLUtriangulatorObj;

typedef void (*_GLUfuncptr)(void);

/* Projection & Matrix utilities */
void gluOrtho2D(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top);
void gluPerspective(GLdouble fovy, GLdouble aspect, GLdouble zNear,
                    GLdouble zFar);
void gluLookAt(GLdouble eyex, GLdouble eyey, GLdouble eyez, GLdouble centerx,
               GLdouble centery, GLdouble centerz, GLdouble upx, GLdouble upy,
               GLdouble upz);
void gluPickMatrix(GLdouble x, GLdouble y, GLdouble delX, GLdouble delY,
                   GLint viewport[4]);
GLint gluProject(GLdouble objX, GLdouble objY, GLdouble objZ,
                 const GLdouble *model, const GLdouble *proj, const GLint *view,
                 GLdouble *winX, GLdouble *winY, GLdouble *winZ);
GLint gluUnProject(GLdouble winX, GLdouble winY, GLdouble winZ,
                   const GLdouble *model, const GLdouble *proj,
                   const GLint *view, GLdouble *objX, GLdouble *objY,
                   GLdouble *objZ);

/* Image utilities */
GLint gluScaleImage(GLenum format, GLsizei wIn, GLsizei hIn, GLenum typeIn,
                    const void *dataIn, GLsizei wOut, GLsizei hOut,
                    GLenum typeOut, GLvoid *dataOut);
GLint gluBuild2DMipmaps(GLenum target, GLint internalFormat, GLsizei width,
                        GLsizei height, GLenum format, GLenum type,
                        const void *data);
const GLubyte *gluErrorString(GLenum error);

/* Quadrics */
GLUquadric *gluNewQuadric(void);
void gluDeleteQuadric(GLUquadric *quad);
void gluQuadricDrawStyle(GLUquadric *quad, GLenum draw);
void gluQuadricNormals(GLUquadric *quad, GLenum normal);
void gluQuadricOrientation(GLUquadric *quad, GLenum orientation);
void gluQuadricTexture(GLUquadric *quad, GLboolean texture);
void gluDisk(GLUquadric *quad, GLdouble inner, GLdouble outer, GLint slices,
             GLint loops);
void gluPartialDisk(GLUquadric *quad, GLdouble inner, GLdouble outer,
                    GLint slices, GLint loops, GLdouble start, GLdouble sweep);
void gluCylinder(GLUquadric *quad, GLdouble base, GLdouble top, GLdouble height,
                 GLint slices, GLint stacks);
void gluSphere(GLUquadric *quad, GLdouble radius, GLint slices, GLint stacks);

/* Tessellation */
GLUtesselator *gluNewTess(void);
void gluDeleteTess(GLUtesselator *tess);
void gluTessProperty(GLUtesselator *tess, GLenum which, GLdouble data);
void gluGetTessProperty(GLUtesselator *tess, GLenum which, GLdouble *data);
void gluTessNormal(GLUtesselator *tess, GLdouble valueX, GLdouble valueY,
                   GLdouble valueZ);
void gluTessCallback(GLUtesselator *tess, GLenum which, _GLUfuncptr CallBackFunc);
void gluTessBeginPolygon(GLUtesselator *tess, GLvoid *data);
void gluTessEndPolygon(GLUtesselator *tess);
void gluTessBeginContour(GLUtesselator *tess);
void gluTessEndContour(GLUtesselator *tess);
void gluTessVertex(GLUtesselator *tess, GLdouble *location, GLvoid *data);

/* Legacy GLU 1.0 polygon tessellation */
void gluBeginPolygon(GLUtesselator *tess);
void gluNextContour(GLUtesselator *tess, GLenum type);
void gluEndPolygon(GLUtesselator *tess);

#ifdef __cplusplus
}
#endif

#endif /* OPENTOONZ_ANDROID_GLU_H */
