#include <stdio.h>
#include <math.h>

#include "vector3d.h"

int main(void)
{
	vec3d A = {1.0, 3.0, 2.0};
	vec3d B = {1.0, 1.0, 1.0};
	vec3d R = {0};

	vec3d_add(A, B, &R);
	print_vec3d(R);

	vec3d_minus(A, B, &R);
	print_vec3d(R);

	vec3d_scalar(2.3, A, &R);
	print_vec3d(R);

	printf("A dot B = %f\n", vec3d_inner(A, B));

	vec3d_cross(A, B, &R);
	print_vec3d(R);
	
	return 0;
}
