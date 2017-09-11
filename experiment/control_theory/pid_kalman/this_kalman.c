#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "kalman.h"

#define SAMPLE_COUNT 512
#define PI 3.1415926535897932384626433832795028841971693993751058209749445923078164062862089986280348253421170679

/*
Sample Data 는 SparkFun IMU 5 자유도 IMU 에서
ATmega168 의 10 비트 ADC 를 사용하여 1 / 20 초(0.05 초) 마다 수집된다.
첫 번째 열은 자이로의 속도(각도/초) 이고 - 한마디로 각속도
두 번째 열은 수평에서의 피치 자세다.
*/

// n 상태값
// m 입력값
// r 출력값

#define n 2
#define m 1
#define r 1

kalman roll_kalman_state;
kalman pitch_kalman_state;

// Kalman 상태 초기화
void kalman_init(kalman *self, float dt, float Sz_00, float Sw_00, float Sw_11)
{
	// 출력 초기화
	self->x_00 = 0.0;
	self->x_10 = 0.0;

	// 공분산의 추정 상태 초기화
	self->P_00 = 3.0;
	self->P_01 = 0.0;
	self->P_10 = 0.0;
	self->P_11 = 3.0;

	// Sample 사이의 델타를 초 단위로 설정
	self->A_00 = 1.0;
	self->A_01 = -dt;
	self->A_10 = 0.0;
	self->A_11 = 1.0;

	self->B_00 = dt;


	// 측정 잡음 공분산 행렬 값을 설정한다.
	self->Sz_00 = Sz_00;

	// 프로세스 잡음 공분산 행렬 값을 설정한다.
	self->Sw_00 = Sw_00;
	self->Sw_11 = Sw_11;
}

float kalman_update(kalman *self,  float gyro_rate,  float accel_angle)
{
	// 센서 값
	float u = gyro_rate;
	float y = accel_angle;

	// 온도
	float s_00;
	float inn_00;
	float K_00;
	float K_10;
	float AP_00;
	float AP_01;
	float AP_10;
	float AP_11;
	float APAT_00;
	float APAT_01;
	float APAT_10;
	float APAT_11;


	/*  현재 상태 추정을 입력 u 로 집어넣어 상태 추정을 갱신한다.
	    x = A * x + B * u
	    추정값 x_00 -> 칼만필터에서 출력값 deg
	    여기선 시간 업데이트 ->> 예측값
	 */
	self->x_00 = (self->A_01 * self->x_10) + (self->B_00 * u);

	/*  추정 상태값과 측정값간의 오차를 계산한다.
	    inn = y - c * x
	    예측값에서 예측한 x_00가 측정 업데이트로 넘어가면서
	 */
	inn_00 = y - self->x_00;

	// 공분산을 계산한다.
	// s = C * P * C' + Sz
	s_00 = self->P_00 + self->Sz_00;

	// AP 행렬을 계산한다.
	// AP = A * P
	AP_00 = self->P_00 + self->A_01 * self->P_10;
	AP_01 = self->P_01 + self->A_01 * self->P_11;
	AP_10 = self->P_10;
	AP_11 = self->P_11;

	// Kalman 이득 행렬을 계산한다.
	// K = A * P * C' * inv(s)
	K_00 = AP_00 / s_00;
	K_10 = AP_10 / s_00;

	// 상태 추정치를 갱신한다.
	// x = x + K * inn
	self->x_00 += K_00 * inn_00;
	self->x_10 += K_10 * inn_00;

	//	printf("k_00 = %f k_10 = %f\n ", K_00, K_10);

	// 추정 오차의 새로운 공분산을 계산한다.
	// P = A * P * A' - K * C * P * A' + Sw
	APAT_00 = AP_00 + (AP_01 * self->A_01);
	APAT_01 = AP_01;
	APAT_10 = AP_10 + (AP_11 * self->A_01);
	APAT_11 = AP_11;

	self->P_00 = APAT_00 + self->Sw_00;
	self->P_01 = APAT_01;
	self->P_10 = APAT_10;
	self->P_11 = APAT_11 + self->Sw_11;

	// 추정치를 반환한다.
	return self->x_00;
}
