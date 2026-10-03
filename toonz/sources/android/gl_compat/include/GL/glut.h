#pragma once
#ifndef OPENTOONZ_ANDROID_GLUT_H
#define OPENTOONZ_ANDROID_GLUT_H

#include <GL/gl.h>
#include <GL/glu.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void *glutStrokeRoman;
extern void *glutStrokeMonoRoman;
extern void *glutBitmap9By15;
extern void *glutBitmap8By13;
extern void *glutBitmapTimesRoman10;
extern void *glutBitmapTimesRoman24;
extern void *glutBitmapHelvetica10;
extern void *glutBitmapHelvetica12;
extern void *glutBitmapHelvetica18;

#define GLUT_STROKE_ROMAN (&glutStrokeRoman)
#define GLUT_STROKE_MONO_ROMAN (&glutStrokeMonoRoman)
#define GLUT_BITMAP_9_BY_15 (&glutBitmap9By15)
#define GLUT_BITMAP_8_BY_13 (&glutBitmap8By13)
#define GLUT_BITMAP_TIMES_ROMAN_10 (&glutBitmapTimesRoman10)
#define GLUT_BITMAP_TIMES_ROMAN_24 (&glutBitmapTimesRoman24)
#define GLUT_BITMAP_HELVETICA_10 (&glutBitmapHelvetica10)
#define GLUT_BITMAP_HELVETICA_12 (&glutBitmapHelvetica12)
#define GLUT_BITMAP_HELVETICA_18 (&glutBitmapHelvetica18)

void glutInit(int *argcp, char **argv);
void glutBitmapCharacter(void *font, int character);
int glutBitmapWidth(void *font, int character);
void glutStrokeCharacter(void *font, int character);
int glutStrokeWidth(void *font, int character);

#ifdef __cplusplus
}
#endif

#endif /* OPENTOONZ_ANDROID_GLUT_H */
