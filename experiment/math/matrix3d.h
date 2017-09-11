#ifndef __MATRIX3D_H__
#define __MATRIX3D_H__

#include "vector3d.h"

typedef struct matrix3d
{
	float data[9];
} mat3d;

void x_rot3d(vec3d A, float angle, vec3d *R);
void y_rot3d(vec3d A, float angle, vec3d *R);
void z_rot3d(vec3d A, float angle, vec3d *R);

#endif
