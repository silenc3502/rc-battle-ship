#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "kalman.h"
#include "kalman_variable.h"

#define PI 3.1415926535897932384626433832795028841971693993751058209749445923078164062862089986280348253421170679

double clk;
clock_t start, end;

void main(void) {
	kalman roll_kalman_state;
	kalman pitch_kalman_state;

	int i, k;
	float roll_gyro_input;
	float pitch_gyro_input;
	float roll_accel_input;
	float pitch_accel_input;
	float roll_kalman_output;
	float pitch_kalman_output;
	float dt = 0.0025;

	srand((unsigned int)time(NULL));

	kalman_init(&roll_kalman_state, dt, 1.002, 0.001, 0.003);
	kalman_init(&pitch_kalman_state, dt, 1.002, 0.001, 0.003);
	
	while(1) 
	{
		
		dt += 0.0025;

		// 자이로 센서 및 가속도 센서 정보 획득
		i = rand() % 30 - 15;
		k = rand() % 30 - 15;
		roll_gyro_input		= (float)i;
		pitch_gyro_input	= (float)k;
		roll_accel_input	= (float)k;
		pitch_accel_input	= (float)i;

		// Kalman Filter 의 갱신 루틴을 구동하여 출력을 얻음
		roll_kalman_output = kalman_update(&roll_kalman_state, roll_gyro_input, roll_accel_input);
		printf(" roll_gyro(x) = %7.3f deg/sec    roll_accel(x)=%7.3f deg    roll_kalman_output(x)=%10.4f deg\n",
			roll_gyro_input, roll_accel_input, roll_kalman_output);

		pitch_kalman_output = kalman_update(&pitch_kalman_state, pitch_gyro_input, pitch_accel_input);
		printf(" pitch_gyro(x)=%7.3f deg/sec    pitch_accel(x)=%7.3f deg    pitch_kalman_output(x)=%10.4f deg\n",
			pitch_gyro_input, pitch_accel_input, pitch_kalman_output);
		pid(roll_kalman_output, pitch_kalman_output);
	
	}
}
