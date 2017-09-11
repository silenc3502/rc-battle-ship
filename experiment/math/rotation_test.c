#include <stdio.h>

#include "matrix3d.h"
#include "vector3d.h"

int main(void)
{
	vec3d A = {1.0, 1.0, 2.0};
	vec3d R = {0};

	z_rot3d(A, 90.0, &R);
	print_vec3d(R);

	y_rot3d(R, 90.0, &R);
	print_vec3d(R);

	z_rot3d(R, 90.0, &R);
	print_vec3d(R);
	
	return 0;
}
