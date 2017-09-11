#include <stdio.h>

#include "quaternion.h"
#include "matrix3d.h"
#include "vector3d.h"

int main(void)
{
	quat A = {0.1, {1.0, 3.0, 2.0}};
	quat B = {0.3, {2.0, 3.0, 6.0}};
	quat C = {1.0, {-1.0, 2.0, -1.0}};
	quat D = {2.0, {-1.0, -2.0, 3.0}};
	quat R = {0};

	vec3d v = {1.0, 1.0, 2.0};
	vec3d r = {0};

	float angle_x;
	float angle_y;
	float angle_z;

	quaternion_add(A, B, &R);
	print_quaternion(R);

	quaternion_minus(A, B, &R);
	print_quaternion(R);

/*
	quaternion_mult(A, B, &R);
	print_quaternion(R);
*/

	quaternion_mult(C, D, &R);
	print_quaternion(R);

	printf("quaternion norm = %.5f\n", quaternion_norm(A));

	quaternion_conjugate(A, &R);
	print_quaternion(R);

	quaternion_reverse(A, &R);
	print_quaternion(R);

	quaternion_mult(A, R, &R);
	print_quaternion(R);

	quaternion_rotation(A, v, &r);
	print_vec3d(r);

	quaternion_from_angle(90.0, 90.0, 90.0, &R);
	print_quaternion(R);

	x_rot3d(v, 90.0, &r);
	y_rot3d(v, 90.0, &r);
	z_rot3d(v, 90.0, &r);
	print_vec3d(r);

	quaternion_rotation(R, v, &r);
	print_vec3d(r);

	angle_from_quaternion(R, &angle_x, &angle_y, &angle_z);
	printf("angle_x = %.5f, angle_y = %.5f, angle_z = %.5f\n", angle_x, angle_y, angle_z);

	return 0;
}
