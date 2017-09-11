#include <stdio.h>

#include "quaternion.h"
#include "vector3d.h"

void quaternion_add(quat A, quat B, quat *R)
{
        R->real_w = A.real_w + B.real_w;
        vec3d_add(A.image_v, B.image_v, &R->image_v);
}

void quaternion_minus(quat A, quat B, quat *R) 
{
        R->real_w = A.real_w - B.real_w;
        vec3d_minus(A.image_v, B.image_v, &R->image_v);
}

void quaternion_mult(quat A, quat B, quat *R)
{
	/*
	R->real_w    = B.real_w    * A.real_w    - B.image_v.x * A.image_v.x
		     - B.image_v.y * A.image_v.y - B.image_v.z * A.image_v.z;
	R->image_v.x = B.real_w    * A.image_v.x + B.image_v.x * A.real_w
		     + B.image_v.y * A.image_v.z - B.image_v.z * A.image_v.y;
	R->image_v.y = B.real_w    * A.image_v.y - B.image_v.x * A.image_v.z
		     + B.image_v.y * A.real_w    + B.image_v.z * A.image_v.x;
	R->image_v.z = B.real_w    * A.image_v.z + B.image_v.x * A.image_v.y
		     - B.image_v.y * A.image_v.x + B.image_v.z * A.real_w;
	*/

	vec3d tmp_cross = {0};
	vec3d tmp_scalar1 = {0};
	vec3d tmp_scalar2 = {0};

	R->real_w = A.real_w * B.real_w - vec3d_inner(A.image_v, B.image_v);

	vec3d_cross(A.image_v, B.image_v, &tmp_cross);
	vec3d_scalar(A.real_w, B.image_v, &tmp_scalar1);
	vec3d_scalar(B.real_w, A.image_v, &tmp_scalar2);

	R->image_v.x = tmp_cross.x + tmp_scalar1.x + tmp_scalar2.x;
	R->image_v.y = tmp_cross.y + tmp_scalar1.y + tmp_scalar2.y;
	R->image_v.z = tmp_cross.z + tmp_scalar1.z + tmp_scalar2.z;
}

float quaternion_norm(quat A)
{
	return sqrt(pow(A.real_w,    2.0) + pow(A.image_v.x, 2.0) +
	            pow(A.image_v.y, 2.0) + pow(A.image_v.z, 2.0));
}

void quaternion_conjugate(quat A, quat *R)
{
	R->real_w = A.real_w;
	R->image_v.x = -A.image_v.x;
	R->image_v.y = -A.image_v.y;
	R->image_v.z = -A.image_v.z;
}

void quaternion_reverse(quat A, quat *R)
{
	float norm_2;
	quat conjugate = {0};

	quaternion_conjugate(A, &conjugate);
	norm_2 = pow(quaternion_norm(A), 2.0);

	R->real_w = conjugate.real_w / norm_2;
	R->image_v.x = conjugate.image_v.x / norm_2;
	R->image_v.y = conjugate.image_v.y / norm_2;
	R->image_v.z = conjugate.image_v.z / norm_2;
}

bool quaternion_isunit(quat A)
{
	float norm = quaternion_norm(A);

	if(norm == 1)
		return true;
	else
		return false;
}

void quaternion_rotation(quat A, vec3d v, vec3d *r)
{
	bool isunit = quaternion_isunit(A);
	float vMult;
	float crossMult;
	float pMult;

	if(isunit)
	{
		vMult = 2.0 * (A.image_v.x * v.x + A.image_v.y * v.y + A.image_v.z * v.z);
		crossMult = 2.0 * A.real_w;
		pMult = crossMult * A.real_w - 1.0;

		r->x = pMult * v.x + vMult * A.image_v.x +
		       crossMult * (A.image_v.y * v.z - A.image_v.z * v.y);
		r->y = pMult * v.y + vMult * A.image_v.y +
		       crossMult * (A.image_v.z * v.x - A.image_v.x * v.z);
		r->z = pMult * v.z + vMult * A.image_v.z +
		       crossMult * (A.image_v.x * v.y - A.image_v.y * v.x);
	}
	else
	{
		quat tmp = {0};
		quaternion_normalization(A, &tmp);

		vMult = 2.0 * (tmp.image_v.x * v.x + tmp.image_v.y * v.y + tmp.image_v.z * v.z);
		crossMult = 2.0 * tmp.real_w;
		pMult = crossMult * tmp.real_w - 1.0;

		r->x = pMult * v.x + vMult * tmp.image_v.x +
		       crossMult * (tmp.image_v.y * v.z - tmp.image_v.z * v.y);
		r->y = pMult * v.y + vMult * tmp.image_v.y +
		       crossMult * (tmp.image_v.z * v.x - tmp.image_v.x * v.z);
		r->z = pMult * v.z + vMult * tmp.image_v.z +
		       crossMult * (tmp.image_v.x * v.y - tmp.image_v.y * v.x);
	}
}

void quaternion_normalization(quat A, quat *R)
{
	float norm = quaternion_norm(A);

	R->real_w = A.real_w / norm;
	R->image_v.x = A.image_v.x / norm;
	R->image_v.y = A.image_v.y / norm;
	R->image_v.z = A.image_v.z / norm;
}

void quaternion_from_angle(float angle_x, float angle_y, float angle_z, quat *R)
{
	float rad_x = (angle_x * (M_PI / 180.0)) / 2.0;
	float rad_y = (angle_y * (M_PI / 180.0)) / 2.0;
	float rad_z = (angle_z * (M_PI / 180.0)) / 2.0;

	R->real_w = cos(rad_x) * cos(rad_y) * cos(rad_z) + sin(rad_x) * sin(rad_y) * sin(rad_z);
	R->image_v.x = sin(rad_x) * cos(rad_y) * cos(rad_z) - cos(rad_x) * sin(rad_y) * sin(rad_z);
	R->image_v.y = cos(rad_x) * sin(rad_y) * cos(rad_z) + sin(rad_x) * cos(rad_y) * sin(rad_z);
	R->image_v.z = cos(rad_x) * cos(rad_y) * sin(rad_z) - sin(rad_x) * sin(rad_y) * cos(rad_z);
}

void angle_from_quaternion(quat A, float *angle_x, float *angle_y, float *angle_z)
{
	*angle_x = (float)(360.0 / (4 * acos(0.0))) * atan(2 * (A.real_w * A.image_v.x + A.image_v.y * A.image_v.z) / (1 - 2 * (A.image_v.x * A.image_v.x + A.image_v.y * A.image_v.y)));
	*angle_y = (float)(360.0 / (4 * acos(0.0))) * asin(2 * (A.real_w * A.image_v.y - A.image_v.x * A.image_v.z));
	*angle_z = (float)(360.0 / (4 * acos(0.0))) * atan(2 * (A.image_v.x * A.image_v.y + A.real_w * A.image_v.z) / (1 - 2 * (A.image_v.y * A.image_v.y + A.image_v.z * A.image_v.z)));
}

void print_quaternion(quat R)
{
	printf("Real w = %.5f, ", R.real_w);
	print_vec3d(R.image_v);
}
