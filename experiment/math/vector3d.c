#include <stdio.h>

#include "vector3d.h"

void vec3d_add(vec3d A, vec3d B, vec3d *res)
{
	res->x = A.x + B.x;
	res->y = A.y + B.y;
	res->z = A.z + B.z;
}

void vec3d_minus(vec3d A, vec3d B, vec3d *res)
{
	res->x = A.x - B.x;
	res->y = A.y - B.y;
	res->z = A.z - B.z;
}

float vec3d_inner(vec3d A, vec3d B)
{
	return A.x * B.x + A.y * B.y + A.z * B.z;
}

void vec3d_cross(vec3d A, vec3d B, vec3d *res)
{
	res->x = A.y * B.z - A.z * B.y;
	res->y = A.z * B.x - A.x * B.z;
	res->z = A.x * B.y - A.y * B.x;
}

void vec3d_scalar(float scalar, vec3d A, vec3d *R)
{
	R->x = scalar * A.x;
	R->y = scalar * A.y;
	R->z = scalar * A.z;
}

void print_vec3d(vec3d R)
{
	printf("x = %.5f, y = %.5f, z = %.5f\n", R.x, R.y, R.z);
}
