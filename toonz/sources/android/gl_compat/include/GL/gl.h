#pragma once
#ifndef OPENTOONZ_ANDROID_GL_H
#define OPENTOONZ_ANDROID_GL_H

#if defined(ANDROID_GL_COMPAT_MOCK_GLES)
#include <stdint.h>
#include <stddef.h>
typedef unsigned int GLenum;
typedef unsigned char GLboolean;
typedef unsigned int GLbitfield;
typedef void GLvoid;
typedef signed char GLbyte;
typedef short GLshort;
typedef int GLint;
typedef int GLclampx;
typedef unsigned char GLubyte;
typedef unsigned short GLushort;
typedef unsigned int GLuint;
typedef int GLsizei;
typedef float GLfloat;
typedef float GLclampf;
typedef double GLdouble;
typedef double GLclampd;
typedef char GLchar;
typedef intptr_t GLintptr;
typedef ptrdiff_t GLsizeiptr;
#ifndef GL_APIENTRY
#define GL_APIENTRY
#endif
#ifndef GL_APICALL
#define GL_APICALL
#endif
#else
#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>
#endif

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Ensure desktop GL double types are always defined */
typedef double GLdouble;
typedef double GLclampd;

/* Boolean values */
#ifndef GL_FALSE
#define GL_FALSE 0
#endif
#ifndef GL_TRUE
#define GL_TRUE 1
#endif

/* Data types */
#ifndef GL_BYTE
#define GL_BYTE 0x1400
#endif
#ifndef GL_UNSIGNED_BYTE
#define GL_UNSIGNED_BYTE 0x1401
#endif
#ifndef GL_SHORT
#define GL_SHORT 0x1402
#endif
#ifndef GL_UNSIGNED_SHORT
#define GL_UNSIGNED_SHORT 0x1403
#endif
#ifndef GL_INT
#define GL_INT 0x1404
#endif
#ifndef GL_UNSIGNED_INT
#define GL_UNSIGNED_INT 0x1405
#endif
#ifndef GL_FLOAT
#define GL_FLOAT 0x1406
#endif
#ifndef GL_2_BYTES
#define GL_2_BYTES 0x1407
#endif
#ifndef GL_3_BYTES
#define GL_3_BYTES 0x1408
#endif
#ifndef GL_4_BYTES
#define GL_4_BYTES 0x1409
#endif
#ifndef GL_DOUBLE
#define GL_DOUBLE 0x140A
#endif

/* Primitives */
#ifndef GL_POINTS
#define GL_POINTS 0x0000
#endif
#ifndef GL_LINES
#define GL_LINES 0x0001
#endif
#ifndef GL_LINE_LOOP
#define GL_LINE_LOOP 0x0002
#endif
#ifndef GL_LINE_STRIP
#define GL_LINE_STRIP 0x0003
#endif
#ifndef GL_TRIANGLES
#define GL_TRIANGLES 0x0004
#endif
#ifndef GL_TRIANGLE_STRIP
#define GL_TRIANGLE_STRIP 0x0005
#endif
#ifndef GL_TRIANGLE_FAN
#define GL_TRIANGLE_FAN 0x0006
#endif
#ifndef GL_QUADS
#define GL_QUADS 0x0007
#endif
#ifndef GL_QUAD_STRIP
#define GL_QUAD_STRIP 0x0008
#endif
#ifndef GL_POLYGON
#define GL_POLYGON 0x0009
#endif

/* Matrix Mode */
#ifndef GL_MATRIX_MODE
#define GL_MATRIX_MODE 0x0BA0
#endif
#ifndef GL_MODELVIEW
#define GL_MODELVIEW 0x1700
#endif
#ifndef GL_PROJECTION
#define GL_PROJECTION 0x1701
#endif
#ifndef GL_TEXTURE
#define GL_TEXTURE 0x1702
#endif
#ifndef GL_COLOR
#define GL_COLOR 0x1800
#endif
#ifndef GL_MODELVIEW_MATRIX
#define GL_MODELVIEW_MATRIX 0x0BA6
#endif
#ifndef GL_PROJECTION_MATRIX
#define GL_PROJECTION_MATRIX 0x0BA7
#endif
#ifndef GL_TEXTURE_MATRIX
#define GL_TEXTURE_MATRIX 0x0BA8
#endif
#ifndef GL_VIEWPORT
#define GL_VIEWPORT 0x0BA2
#endif

/* Vertex Arrays */
#ifndef GL_VERTEX_ARRAY
#define GL_VERTEX_ARRAY 0x8074
#endif
#ifndef GL_NORMAL_ARRAY
#define GL_NORMAL_ARRAY 0x8075
#endif
#ifndef GL_COLOR_ARRAY
#define GL_COLOR_ARRAY 0x8076
#endif
#ifndef GL_INDEX_ARRAY
#define GL_INDEX_ARRAY 0x8077
#endif
#ifndef GL_TEXTURE_COORD_ARRAY
#define GL_TEXTURE_COORD_ARRAY 0x8078
#endif
#ifndef GL_EDGE_FLAG_ARRAY
#define GL_EDGE_FLAG_ARRAY 0x8079
#endif

/* Render Mode & Selection */
#ifndef GL_RENDER
#define GL_RENDER 0x1C00
#endif
#ifndef GL_FEEDBACK
#define GL_FEEDBACK 0x1C01
#endif
#ifndef GL_SELECT
#define GL_SELECT 0x1C02
#endif
#ifndef GL_RENDER_MODE
#define GL_RENDER_MODE 0x0C40
#endif
#ifndef GL_NAME_STACK_DEPTH
#define GL_NAME_STACK_DEPTH 0x0D70
#endif
#ifndef GL_MAX_NAME_STACK_DEPTH
#define GL_MAX_NAME_STACK_DEPTH 0x0D37
#endif

/* Display Lists */
#ifndef GL_COMPILE
#define GL_COMPILE 0x1300
#endif
#ifndef GL_COMPILE_AND_EXECUTE
#define GL_COMPILE_AND_EXECUTE 0x1301
#endif
#ifndef GL_LIST_BASE
#define GL_LIST_BASE 0x0B32
#endif
#ifndef GL_LIST_INDEX
#define GL_LIST_INDEX 0x0B33
#endif
#ifndef GL_LIST_MODE
#define GL_LIST_MODE 0x0B30
#endif

/* Lighting */
#ifndef GL_LIGHTING
#define GL_LIGHTING 0x0B50
#endif
#ifndef GL_LIGHT0
#define GL_LIGHT0 0x4000
#endif
#ifndef GL_LIGHT1
#define GL_LIGHT1 0x4001
#endif
#ifndef GL_LIGHT_MODEL_LOCAL_VIEWER
#define GL_LIGHT_MODEL_LOCAL_VIEWER 0x0B51
#endif
#ifndef GL_LIGHT_MODEL_TWO_SIDE
#define GL_LIGHT_MODEL_TWO_SIDE 0x0B52
#endif
#ifndef GL_LIGHT_MODEL_AMBIENT
#define GL_LIGHT_MODEL_AMBIENT 0x0B53
#endif
#ifndef GL_AMBIENT
#define GL_AMBIENT 0x1200
#endif
#ifndef GL_DIFFUSE
#define GL_DIFFUSE 0x1201
#endif
#ifndef GL_SPECULAR
#define GL_SPECULAR 0x1202
#endif
#ifndef GL_POSITION
#define GL_POSITION 0x1203
#endif
#ifndef GL_EMISSION
#define GL_EMISSION 0x1600
#endif
#ifndef GL_SHININESS
#define GL_SHININESS 0x1601
#endif
#ifndef GL_AMBIENT_AND_DIFFUSE
#define GL_AMBIENT_AND_DIFFUSE 0x1602
#endif
#ifndef GL_COLOR_MATERIAL
#define GL_COLOR_MATERIAL 0x0B57
#endif
#ifndef GL_NORMALIZE
#define GL_NORMALIZE 0x0BA1
#endif
#ifndef GL_FLAT
#define GL_FLAT 0x1D00
#endif
#ifndef GL_SMOOTH
#define GL_SMOOTH 0x1D01
#endif

/* Alpha / Blending / LogicOp / Depth / Stencil */
#ifndef GL_ALPHA_TEST
#define GL_ALPHA_TEST 0x0BC0
#endif
#ifndef GL_ALPHA_TEST_REF
#define GL_ALPHA_TEST_REF 0x0BC2
#endif
#ifndef GL_ALPHA_TEST_FUNC
#define GL_ALPHA_TEST_FUNC 0x0BC1
#endif
#ifndef GL_BLEND
#define GL_BLEND 0x0BE2
#endif
#ifndef GL_BLEND_SRC
#define GL_BLEND_SRC 0x0BE1
#endif
#ifndef GL_BLEND_DST
#define GL_BLEND_DST 0x0BE0
#endif
#ifndef GL_ZERO
#define GL_ZERO 0
#endif
#ifndef GL_ONE
#define GL_ONE 1
#endif
#ifndef GL_SRC_COLOR
#define GL_SRC_COLOR 0x0300
#endif
#ifndef GL_ONE_MINUS_SRC_COLOR
#define GL_ONE_MINUS_SRC_COLOR 0x0301
#endif
#ifndef GL_SRC_ALPHA
#define GL_SRC_ALPHA 0x0302
#endif
#ifndef GL_ONE_MINUS_SRC_ALPHA
#define GL_ONE_MINUS_SRC_ALPHA 0x0303
#endif
#ifndef GL_DST_ALPHA
#define GL_DST_ALPHA 0x0304
#endif
#ifndef GL_ONE_MINUS_DST_ALPHA
#define GL_ONE_MINUS_DST_ALPHA 0x0305
#endif
#ifndef GL_DST_COLOR
#define GL_DST_COLOR 0x0306
#endif
#ifndef GL_ONE_MINUS_DST_COLOR
#define GL_ONE_MINUS_DST_COLOR 0x0307
#endif
#ifndef GL_SRC_ALPHA_SATURATE
#define GL_SRC_ALPHA_SATURATE 0x0308
#endif
#ifndef GL_LOGIC_OP
#define GL_LOGIC_OP 0x0BF1
#endif
#ifndef GL_COLOR_LOGIC_OP
#define GL_COLOR_LOGIC_OP 0x0BF2
#endif
#ifndef GL_INVERT
#define GL_INVERT 0x150A
#endif
#ifndef GL_XOR
#define GL_XOR 0x1506
#endif
#ifndef GL_DITHER
#define GL_DITHER 0x0BD0
#endif
#ifndef GL_DEPTH_TEST
#define GL_DEPTH_TEST 0x0B71
#endif
#ifndef GL_STENCIL_TEST
#define GL_STENCIL_TEST 0x0B90
#endif
#ifndef GL_STENCIL_BITS
#define GL_STENCIL_BITS 0x0D57
#endif
#ifndef GL_SCISSOR_TEST
#define GL_SCISSOR_TEST 0x0C11
#endif
#ifndef GL_CULL_FACE
#define GL_CULL_FACE 0x0B44
#endif
#ifndef GL_NEVER
#define GL_NEVER 0x0200
#endif
#ifndef GL_LESS
#define GL_LESS 0x0201
#endif
#ifndef GL_EQUAL
#define GL_EQUAL 0x0202
#endif
#ifndef GL_LEQUAL
#define GL_LEQUAL 0x0203
#endif
#ifndef GL_GREATER
#define GL_GREATER 0x0204
#endif
#ifndef GL_NOTEQUAL
#define GL_NOTEQUAL 0x0205
#endif
#ifndef GL_GEQUAL
#define GL_GEQUAL 0x0206
#endif
#ifndef GL_ALWAYS
#define GL_ALWAYS 0x0207
#endif
#ifndef GL_KEEP
#define GL_KEEP 0x1E00
#endif
#ifndef GL_REPLACE
#define GL_REPLACE 0x1E01
#endif
#ifndef GL_INCR
#define GL_INCR 0x1E02
#endif
#ifndef GL_DECR
#define GL_DECR 0x1E03
#endif

/* Buffers & Clear bits */
#ifndef GL_DEPTH_BUFFER_BIT
#define GL_DEPTH_BUFFER_BIT 0x00000100
#endif
#ifndef GL_ACCUM_BUFFER_BIT
#define GL_ACCUM_BUFFER_BIT 0x00000200
#endif
#ifndef GL_STENCIL_BUFFER_BIT
#define GL_STENCIL_BUFFER_BIT 0x00000400
#endif
#ifndef GL_COLOR_BUFFER_BIT
#define GL_COLOR_BUFFER_BIT 0x00004000
#endif

/* Attrib Mask Bits */
#ifndef GL_CURRENT_BIT
#define GL_CURRENT_BIT 0x00000001
#endif
#ifndef GL_POINT_BIT
#define GL_POINT_BIT 0x00000002
#endif
#ifndef GL_LINE_BIT
#define GL_LINE_BIT 0x00000004
#endif
#ifndef GL_POLYGON_BIT
#define GL_POLYGON_BIT 0x00000008
#endif
#ifndef GL_POLYGON_STIPPLE_BIT
#define GL_POLYGON_STIPPLE_BIT 0x00000010
#endif
#ifndef GL_PIXEL_MODE_BIT
#define GL_PIXEL_MODE_BIT 0x00000020
#endif
#ifndef GL_LIGHTING_BIT
#define GL_LIGHTING_BIT 0x00000040
#endif
#ifndef GL_FOG_BIT
#define GL_FOG_BIT 0x00000080
#endif
#ifndef GL_VIEWPORT_BIT
#define GL_VIEWPORT_BIT 0x00000800
#endif
#ifndef GL_TRANSFORM_BIT
#define GL_TRANSFORM_BIT 0x00001000
#endif
#ifndef GL_ENABLE_BIT
#define GL_ENABLE_BIT 0x00002000
#endif
#ifndef GL_HINT_BIT
#define GL_HINT_BIT 0x00008000
#endif
#ifndef GL_EVAL_BIT
#define GL_EVAL_BIT 0x00010000
#endif
#ifndef GL_LIST_BIT
#define GL_LIST_BIT 0x00020000
#endif
#ifndef GL_TEXTURE_BIT
#define GL_TEXTURE_BIT 0x00040000
#endif
#ifndef GL_SCISSOR_BIT
#define GL_SCISSOR_BIT 0x00080000
#endif
#ifndef GL_ALL_ATTRIB_BITS
#define GL_ALL_ATTRIB_BITS 0x000FFFFF
#endif

/* Polygon / Line / Point state */
#ifndef GL_POINT
#define GL_POINT 0x1B00
#endif
#ifndef GL_LINE
#define GL_LINE 0x1B01
#endif
#ifndef GL_FILL
#define GL_FILL 0x1B02
#endif
#ifndef GL_FRONT
#define GL_FRONT 0x0404
#endif
#ifndef GL_BACK
#define GL_BACK 0x0405
#endif
#ifndef GL_FRONT_AND_BACK
#define GL_FRONT_AND_BACK 0x0408
#endif
#ifndef GL_NONE
#define GL_NONE 0
#endif
#ifndef GL_LINE_SMOOTH
#define GL_LINE_SMOOTH 0x0B20
#endif
#ifndef GL_LINE_SMOOTH_HINT
#define GL_LINE_SMOOTH_HINT 0x0C52
#endif
#ifndef GL_LINE_STIPPLE
#define GL_LINE_STIPPLE 0x0B24
#endif
#ifndef GL_LINE_WIDTH
#define GL_LINE_WIDTH 0x0B21
#endif
#ifndef GL_LINE_WIDTH_RANGE
#define GL_LINE_WIDTH_RANGE 0x0B22
#endif
#ifndef GL_POINT_SMOOTH
#define GL_POINT_SMOOTH 0x0B10
#endif
#ifndef GL_POINT_SMOOTH_HINT
#define GL_POINT_SMOOTH_HINT 0x0C51
#endif
#ifndef GL_POINT_SIZE
#define GL_POINT_SIZE 0x0B11
#endif
#ifndef GL_POINT_SIZE_RANGE
#define GL_POINT_SIZE_RANGE 0x0B12
#endif
#ifndef GL_POLYGON_SMOOTH
#define GL_POLYGON_SMOOTH 0x0B41
#endif
#ifndef GL_POLYGON_SMOOTH_HINT
#define GL_POLYGON_SMOOTH_HINT 0x0C53
#endif
#ifndef GL_PERSPECTIVE_CORRECTION_HINT
#define GL_PERSPECTIVE_CORRECTION_HINT 0x0C50
#endif
#ifndef GL_FASTEST
#define GL_FASTEST 0x1101
#endif
#ifndef GL_NICEST
#define GL_NICEST 0x1102
#endif
#ifndef GL_DONT_CARE
#define GL_DONT_CARE 0x1100
#endif

/* State Getters */
#ifndef GL_CURRENT_COLOR
#define GL_CURRENT_COLOR 0x0B00
#endif
#ifndef GL_CURRENT_RASTER_POSITION
#define GL_CURRENT_RASTER_POSITION 0x0B07
#endif
#ifndef GL_COLOR_WRITEMASK
#define GL_COLOR_WRITEMASK 0x0C23
#endif
#ifndef GL_UNPACK_ROW_LENGTH
#define GL_UNPACK_ROW_LENGTH 0x0CF2
#endif
#ifndef GL_UNPACK_ALIGNMENT
#define GL_UNPACK_ALIGNMENT 0x0CF5
#endif
#ifndef GL_PACK_ROW_LENGTH
#define GL_PACK_ROW_LENGTH 0x0D02
#endif
#ifndef GL_PACK_ALIGNMENT
#define GL_PACK_ALIGNMENT 0x0D05
#endif

/* Evaluators */
#ifndef GL_MAP1_VERTEX_3
#define GL_MAP1_VERTEX_3 0x0D97
#endif
#ifndef GL_MAP1_VERTEX_4
#define GL_MAP1_VERTEX_4 0x0D98
#endif

/* Textures & Pixel Formats */
#ifndef GL_TEXTURE_1D
#define GL_TEXTURE_1D 0x0DE0
#endif
#ifndef GL_TEXTURE_2D
#define GL_TEXTURE_2D 0x0DE1
#endif
#ifndef GL_TEXTURE_3D_EXT
#define GL_TEXTURE_3D_EXT 0x806F
#endif
#ifndef GL_TEXTURE_RECTANGLE_EXT
#define GL_TEXTURE_RECTANGLE_EXT 0x84F5
#endif
#ifndef GL_PROXY_TEXTURE_2D
#define GL_PROXY_TEXTURE_2D 0x8064
#endif
#ifndef GL_MAX_TEXTURE_SIZE
#define GL_MAX_TEXTURE_SIZE 0x0D33
#endif
#ifndef GL_TEXTURE_WIDTH
#define GL_TEXTURE_WIDTH 0x1000
#endif
#ifndef GL_TEXTURE_HEIGHT
#define GL_TEXTURE_HEIGHT 0x1001
#endif
#ifndef GL_TEXTURE_INTERNAL_FORMAT
#define GL_TEXTURE_INTERNAL_FORMAT 0x1003
#endif
#ifndef GL_TEXTURE_COMPONENTS
#define GL_TEXTURE_COMPONENTS GL_TEXTURE_INTERNAL_FORMAT
#endif
#ifndef GL_TEXTURE_RED_SIZE
#define GL_TEXTURE_RED_SIZE 0x805C
#endif
#ifndef GL_TEXTURE_GREEN_SIZE
#define GL_TEXTURE_GREEN_SIZE 0x805D
#endif
#ifndef GL_TEXTURE_BLUE_SIZE
#define GL_TEXTURE_BLUE_SIZE 0x805E
#endif
#ifndef GL_TEXTURE_ALPHA_SIZE
#define GL_TEXTURE_ALPHA_SIZE 0x805F
#endif
#ifndef GL_TEXTURE_MAG_FILTER
#define GL_TEXTURE_MAG_FILTER 0x2800
#endif
#ifndef GL_TEXTURE_MIN_FILTER
#define GL_TEXTURE_MIN_FILTER 0x2801
#endif
#ifndef GL_TEXTURE_WRAP_S
#define GL_TEXTURE_WRAP_S 0x2802
#endif
#ifndef GL_TEXTURE_WRAP_T
#define GL_TEXTURE_WRAP_T 0x2803
#endif
#ifndef GL_NEAREST
#define GL_NEAREST 0x2600
#endif
#ifndef GL_LINEAR
#define GL_LINEAR 0x2601
#endif
#ifndef GL_CLAMP
#define GL_CLAMP 0x2900
#endif
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif
#ifndef GL_REPEAT
#define GL_REPEAT 0x2901
#endif
#ifndef GL_TEXTURE_ENV
#define GL_TEXTURE_ENV 0x2300
#endif
#ifndef GL_TEXTURE_ENV_MODE
#define GL_TEXTURE_ENV_MODE 0x2200
#endif
#ifndef GL_TEXTURE_ENV_COLOR
#define GL_TEXTURE_ENV_COLOR 0x2201
#endif
#ifndef GL_MODULATE
#define GL_MODULATE 0x2100
#endif
#ifndef GL_DECAL
#define GL_DECAL 0x2101
#endif
#ifndef GL_TEXTURE0
#define GL_TEXTURE0 0x84C0
#endif
#ifndef GL_TEXTURE1
#define GL_TEXTURE1 0x84C1
#endif
#ifndef GL_TEXTURE2
#define GL_TEXTURE2 0x84C2
#endif

#ifndef GL_LUMINANCE
#define GL_LUMINANCE 0x1909
#endif
#ifndef GL_LUMINANCE_ALPHA
#define GL_LUMINANCE_ALPHA 0x190A
#endif
#ifndef GL_ALPHA
#define GL_ALPHA 0x1906
#endif
#ifndef GL_RGB
#define GL_RGB 0x1907
#endif
#ifndef GL_RGBA
#define GL_RGBA 0x1908
#endif
#ifndef GL_RGBA8
#define GL_RGBA8 0x8058
#endif
#ifndef GL_RGBA16
#define GL_RGBA16 0x805B
#endif
#ifndef GL_RGB10_A2
#define GL_RGB10_A2 0x8059
#endif
#ifndef GL_BGR
#define GL_BGR 0x80E0
#endif
#ifndef GL_BGRA
#define GL_BGRA 0x80E1
#endif
#ifndef GL_BGR_EXT
#define GL_BGR_EXT 0x80E0
#endif
#ifndef GL_BGRA_EXT
#define GL_BGRA_EXT 0x80E1
#endif
#ifndef GL_ABGR_EXT
#define GL_ABGR_EXT 0x8000
#endif
#ifndef GL_UNSIGNED_INT_8_8_8_8
#define GL_UNSIGNED_INT_8_8_8_8 0x8035
#endif
#ifndef GL_UNSIGNED_INT_8_8_8_8_REV
#define GL_UNSIGNED_INT_8_8_8_8_REV 0x8367
#endif

/* Imaging subset extensions */
#ifndef GL_CONVOLUTION_1D_EXT
#define GL_CONVOLUTION_1D_EXT 0x8010
#endif
#ifndef GL_CONVOLUTION_2D_EXT
#define GL_CONVOLUTION_2D_EXT 0x8011
#endif
#ifndef GL_SEPARABLE_2D_EXT
#define GL_SEPARABLE_2D_EXT 0x8012
#endif
#ifndef GL_HISTOGRAM_EXT
#define GL_HISTOGRAM_EXT 0x8024
#endif
#ifndef GL_MINMAX_EXT
#define GL_MINMAX_EXT 0x802E
#endif

/* Framebuffers & Transform Feedback & Queries */
#ifndef GL_FRAMEBUFFER
#define GL_FRAMEBUFFER 0x8D40
#endif
#ifndef GL_FRAMEBUFFER_COMPLETE
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5
#endif
#ifndef GL_FRAMEBUFFER_UNDEFINED
#define GL_FRAMEBUFFER_UNDEFINED 0x8219
#endif
#ifndef GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT
#define GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT 0x8CD6
#endif
#ifndef GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT
#define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT 0x8CD7
#endif
#ifndef GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER
#define GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER 0x8CDB
#endif
#ifndef GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER
#define GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER 0x8CDC
#endif
#ifndef GL_FRAMEBUFFER_UNSUPPORTED
#define GL_FRAMEBUFFER_UNSUPPORTED 0x8CDD
#endif
#ifndef GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE
#define GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE 0x8D56
#endif
#ifndef GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS
#define GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS 0x8DA8
#endif
#ifndef GL_ARRAY_BUFFER
#define GL_ARRAY_BUFFER 0x8892
#endif
#ifndef GL_STATIC_READ
#define GL_STATIC_READ 0x88E5
#endif
#ifndef GL_TRANSFORM_FEEDBACK_BUFFER
#define GL_TRANSFORM_FEEDBACK_BUFFER 0x8C8E
#endif
#ifndef GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN
#define GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN 0x8C88
#endif
#ifndef GL_RASTERIZER_DISCARD
#define GL_RASTERIZER_DISCARD 0x8C89
#endif
#ifndef GL_INTERLEAVED_ATTRIBS
#define GL_INTERLEAVED_ATTRIBS 0x8C8C
#endif
#ifndef GL_SEPARATE_ATTRIBS
#define GL_SEPARATE_ATTRIBS 0x8C8D
#endif
#ifndef GL_QUERY_RESULT
#define GL_QUERY_RESULT 0x8866
#endif


/* Internal texture format enums used by the ES upload paths */
#ifndef GL_R8
#define GL_R8 0x8229
#endif
#ifndef GL_RG8
#define GL_RG8 0x822B
#endif
#ifndef GL_RGB8
#define GL_RGB8 0x8051
#endif
#ifndef GL_TEXTURE_BINDING_2D
#define GL_TEXTURE_BINDING_2D 0x8069
#endif
#ifndef GL_ALIASED_LINE_WIDTH_RANGE
#define GL_ALIASED_LINE_WIDTH_RANGE 0x846E
#endif

/* Shader objects (GLES 3.0) */
#ifndef GL_VERTEX_SHADER
#define GL_VERTEX_SHADER 0x8B31
#endif
#ifndef GL_FRAGMENT_SHADER
#define GL_FRAGMENT_SHADER 0x8B30
#endif
#ifndef GL_COMPILE_STATUS
#define GL_COMPILE_STATUS 0x8B81
#endif
#ifndef GL_LINK_STATUS
#define GL_LINK_STATUS 0x8B82
#endif
#ifndef GL_TRUE
#define GL_TRUE 1
#endif
#ifndef GL_FALSE
#define GL_FALSE 0
#endif

/* Errors & Strings */
#ifndef GL_NO_ERROR
#define GL_NO_ERROR 0
#endif
#ifndef GL_INVALID_ENUM
#define GL_INVALID_ENUM 0x0500
#endif
#ifndef GL_INVALID_VALUE
#define GL_INVALID_VALUE 0x0501
#endif
#ifndef GL_INVALID_OPERATION
#define GL_INVALID_OPERATION 0x0502
#endif
#ifndef GL_STACK_OVERFLOW
#define GL_STACK_OVERFLOW 0x0503
#endif
#ifndef GL_STACK_UNDERFLOW
#define GL_STACK_UNDERFLOW 0x0504
#endif
#ifndef GL_OUT_OF_MEMORY
#define GL_OUT_OF_MEMORY 0x0505
#endif
#ifndef GL_INVALID_FRAMEBUFFER_OPERATION
#define GL_INVALID_FRAMEBUFFER_OPERATION 0x0506
#endif
#ifndef GL_VENDOR
#define GL_VENDOR 0x1F00
#endif
#ifndef GL_RENDERER
#define GL_RENDERER 0x1F01
#endif
#ifndef GL_VERSION
#define GL_VERSION 0x1F02
#endif
#ifndef GL_EXTENSIONS
#define GL_EXTENSIONS 0x1F03
#endif

/* Immediate mode & Vertex specification */
void glBegin(GLenum mode);
void glEnd(void);

void glVertex2d(GLdouble x, GLdouble y);
void glVertex2f(GLfloat x, GLfloat y);
void glVertex2i(GLint x, GLint y);
void glVertex2s(GLshort x, GLshort y);
void glVertex3d(GLdouble x, GLdouble y, GLdouble z);
void glVertex3f(GLfloat x, GLfloat y, GLfloat z);
void glVertex3i(GLint x, GLint y, GLint z);
void glVertex4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w);
void glVertex4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
void glVertex2dv(const GLdouble *v);
void glVertex2fv(const GLfloat *v);
void glVertex3dv(const GLdouble *v);
void glVertex3fv(const GLfloat *v);
void glVertex4dv(const GLdouble *v);
void glVertex4fv(const GLfloat *v);

void glColor3b(GLbyte red, GLbyte green, GLbyte blue);
void glColor3d(GLdouble red, GLdouble green, GLdouble blue);
void glColor3f(GLfloat red, GLfloat green, GLfloat blue);
void glColor3ub(GLubyte red, GLubyte green, GLubyte blue);
void glColor4b(GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha);
void glColor4d(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha);
void glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
void glColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
void glColor3dv(const GLdouble *v);
void glColor3fv(const GLfloat *v);
void glColor3ubv(const GLubyte *v);
void glColor4dv(const GLdouble *v);
void glColor4fv(const GLfloat *v);
void glColor4ubv(const GLubyte *v);

void glTexCoord1d(GLdouble s);
void glTexCoord1f(GLfloat s);
void glTexCoord2d(GLdouble s, GLdouble t);
void glTexCoord2f(GLfloat s, GLfloat t);
void glTexCoord2i(GLint s, GLint t);
void glTexCoord2dv(const GLdouble *v);
void glTexCoord2fv(const GLfloat *v);

void glNormal3d(GLdouble nx, GLdouble ny, GLdouble nz);
void glNormal3f(GLfloat nx, GLfloat ny, GLfloat nz);
void glNormal3dv(const GLdouble *v);
void glNormal3fv(const GLfloat *v);

void glRectd(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2);
void glRectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2);
void glRecti(GLint x1, GLint y1, GLint x2, GLint y2);

/* Matrix stack functions */
void glMatrixMode(GLenum mode);
void glLoadIdentity(void);
void glPushMatrix(void);
void glPopMatrix(void);
void glLoadMatrixf(const GLfloat *m);
void glLoadMatrixd(const GLdouble *m);
void glMultMatrixf(const GLfloat *m);
void glMultMatrixd(const GLdouble *m);
void glTranslatef(GLfloat x, GLfloat y, GLfloat z);
void glTranslated(GLdouble x, GLdouble y, GLdouble z);
void glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
void glRotated(GLdouble angle, GLdouble x, GLdouble y, GLdouble z);
void glScalef(GLfloat x, GLfloat y, GLfloat z);
void glScaled(GLdouble x, GLdouble y, GLdouble z);
void glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top,
             GLdouble near_val, GLdouble far_val);
void glFrustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top,
               GLdouble near_val, GLdouble far_val);

/* Attribute & Client State stack */
void glPushAttrib(GLbitfield mask);
void glPopAttrib(void);
void glEnableClientState(GLenum cap);
void glDisableClientState(GLenum cap);
void glVertexPointer(GLint size, GLenum type, GLsizei stride,
                     const GLvoid *ptr);
void glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr);
void glTexCoordPointer(GLint size, GLenum type, GLsizei stride,
                       const GLvoid *ptr);
void glNormalPointer(GLenum type, GLsizei stride, const GLvoid *ptr);
void glArrayElement(GLint i);

/* Legacy client-array draw entry point (GLES only has the
   vertex-attrib based counterpart) */
void glDrawArrays(GLenum mode, GLint first, GLsizei count);

/* Display Lists */
GLuint glGenLists(GLsizei range);
void glDeleteLists(GLuint list, GLsizei range);
void glNewList(GLuint list, GLenum mode);
void glEndList(void);
void glCallList(GLuint list);
void glCallLists(GLsizei n, GLenum type, const GLvoid *lists);
void glListBase(GLuint base);
GLboolean glIsList(GLuint list);

/* Raster / Pixel / Bitmap operations */
void glRasterPos2d(GLdouble x, GLdouble y);
void glRasterPos2f(GLfloat x, GLfloat y);
void glRasterPos2i(GLint x, GLint y);
void glRasterPos3d(GLdouble x, GLdouble y, GLdouble z);
void glRasterPos3f(GLfloat x, GLfloat y, GLfloat z);
void glRasterPos4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
void glRasterPos4fv(const GLfloat *v);
void glBitmap(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig,
              GLfloat xmove, GLfloat ymove, const GLubyte *bitmap);
void glDrawPixels(GLsizei width, GLsizei height, GLenum format, GLenum type,
                  const GLvoid *pixels);
void glCopyPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type);
void glPixelZoom(GLfloat xfactor, GLfloat yfactor);

/* Selection & Picking */
GLint glRenderMode(GLenum mode);
void glSelectBuffer(GLsizei size, GLuint *buffer);
void glInitNames(void);
void glLoadName(GLuint name);
void glPushName(GLuint name);
void glPopName(void);

/* Fixed-function state & lighting */
void glAlphaFunc(GLenum func, GLclampf ref);
void glPolygonMode(GLenum face, GLenum mode);
void glLineStipple(GLint factor, GLushort pattern);
void glPointSize(GLfloat size);
void glLogicOp(GLenum opcode);
void glTexEnvf(GLenum target, GLenum pname, GLfloat param);
void glTexEnvi(GLenum target, GLenum pname, GLint param);
void glTexEnvfv(GLenum target, GLenum pname, const GLfloat *params);
void glLightfv(GLenum light, GLenum pname, const GLfloat *params);
void glLightf(GLenum light, GLenum pname, GLfloat param);
void glLightModeli(GLenum pname, GLint param);
void glLightModelfv(GLenum pname, const GLfloat *params);
void glMaterialfv(GLenum face, GLenum pname, const GLfloat *params);
void glMaterialf(GLenum face, GLenum pname, GLfloat param);
void glShadeModel(GLenum mode);
void glDrawBuffer(GLenum mode);
void glReadBuffer(GLenum mode);

/* 1D Evaluators */
void glMap1f(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order,
             const GLfloat *points);
void glMap1d(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order,
             const GLdouble *points);
void glMapGrid1f(GLint un, GLfloat u1, GLfloat u2);
void glMapGrid1d(GLint un, GLdouble u1, GLdouble u2);
void glEvalMesh1(GLenum mode, GLint i1, GLint i2);
void glEvalCoord1f(GLfloat u);
void glEvalCoord1d(GLdouble u);

/* Queries & Buffer SubData not in GLES 3.0 core */
GLenum glGetError(void);
void glGetDoublev(GLenum pname, GLdouble *params);
void glGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname,
                              GLint *params);
void glGetQueryObjectiv(GLuint id, GLenum pname, GLint *params);
void glGetBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size,
                        GLvoid *data);

#ifdef __cplusplus
}
#endif

#endif /* OPENTOONZ_ANDROID_GL_H */
