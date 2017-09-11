#include <stdio.h>
#include <math.h>

#include "matrix3d.h"
#include "vector3d.h"

void x_rot3d(vec3d A, float angle, vec3d *R)
{
	float rad = angle * (M_PI / 180.0);

	R->x = A.x;
	R->y = A.y * cos(rad) + A.z * sin(rad);
	R->z = A.y * -sin(rad) + A.z * cos(rad);
}

void y_rot3d(vec3d A, float angle, vec3d *R)
{
	float rad = angle * (M_PI / 180.0);

	R->x = A.x * cos(rad) - A.z * sin(rad);
	R->y = A.y;
	R->z = A.x * sin(rad) + A.z * cos(rad);
}

void z_rot3d(vec3d A, float angle, vec3d *R)
{
	float rad = angle * (M_PI / 180.0);

	R->x = A.x * cos(rad) + A.y * sin(rad);
	R->y = A.x * -sin(rad) + A.y * cos(rad);
	R->z = A.z;
}
