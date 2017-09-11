#ifndef __VECTOR3D_H__
#define __VECTOR3D_H__

typedef struct vector3d
{
	float x;
	float y;
	float z;
} vec3d;

void vec3d_add(vec3d A, vec3d B, vec3d *res);
void vec3d_minus(vec3d A, vec3d B, vec3d *res);
float vec3d_inner(vec3d A, vec3d B);
void vec3d_scalar(float scalar, vec3d A, vec3d *res);
void vec3d_cross(vec3d A, vec3d B, vec3d *res);
void print_vec3d(vec3d R);

#endif
