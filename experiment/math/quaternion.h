#ifndef __QUATERNION_H__
#define __QUATERNION_H__

#include "vector3d.h"
#include <math.h>

typedef enum {false, true} bool;

typedef struct quaternion
{
	float real_w;
	vec3d image_v;
} quat;

void quaternion_add(quat A, quat B, quat *R);
void quaternion_minus(quat A, quat B, quat *R);
void quaternion_mult(quat A, quat B, quat *R);
float quaternion_norm(quat A);
void quaternion_conjugate(quat A, quat *R);
void quaternion_reverse(quat A, quat *R);
bool quaternion_isunit(quat A);
void quaternion_rotation(quat A, vec3d v, vec3d *r);
void quaternion_normalization(quat A, quat *R);
void quaternion_from_angle(float angle_x, float angle_y, float angle_z, quat *R);
void angle_from_quaternion(quat A, float *angle_x, float *angle_y, float *angle_z);
void print_quaternion(quat R);

#endif
