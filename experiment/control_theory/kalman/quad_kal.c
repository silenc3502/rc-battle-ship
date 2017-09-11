#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "kalman.h"

#define PI 3.1415926535897932384626433832795028841971693993751058209749445923078164062862089986280348253421170679

double clk;
clock_t start, end;

int main(void) {
	kalman roll_kalman_state;
	kalman pitch_kalman_state;

	int i, k;
	float roll_gyro_input;
	float pitch_gyro_input;
	float roll_accel_input;
	float pitch_accel_input;
	float roll_kalman_output;
	float pitch_kalman_output;
	float dt = 0.00098147;

	srand((unsigned int)time(NULL));

	kalman_init(&roll_kalman_state, dt, 0.00002, 0.001, 0.003);
	kalman_init(&pitch_kalman_state, dt, 0.00002, 0.001, 0.003);

	while (1) {

		dt += 0.00098147;

		// 자이로 및 가속도 센서값 얻기
		i = rand() % 50 - 25
			;
		k = rand() % 20 - 10;
		roll_gyro_input = (float)i;
		pitch_gyro_input = (float)k;
		roll_accel_input = 5 + 10 * sin(2 * PI * dt);
		pitch_accel_input = 5 + 10 * cos(2 * PI * dt);

		roll_kalman_output = kalman_update(&roll_kalman_state, roll_gyro_input, roll_accel_input);
		printf(" roll_gyro(x) = %7.3f deg/sec    roll_accel(x)=%7.3f deg    roll_kalman_output(x)=%10.4f deg\n",
				roll_gyro_input, roll_accel_input, roll_kalman_output);

		pitch_kalman_output = kalman_update(&pitch_kalman_state, pitch_gyro_input, pitch_accel_input);
		printf(" pitch_gyro(x)=%7.3f deg/sec    pitch_accel(x)=%7.3f deg    pitch_kalman_output(x)=%10.4f deg\n",
				pitch_gyro_input, pitch_accel_input, pitch_kalman_output);

	}

	return 0;
}
