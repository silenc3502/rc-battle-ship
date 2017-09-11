typedef struct _kalman kalman;

struct _kalman
{
	float x_00;
	float x_10;	
			
	float P_00;
	float P_01;
	float P_10;
	float P_11;	

	float A_00;
	float A_01;
	float A_10;
	float A_11;

	float B_00;

	float Sw_00;
	float Sw_11;

	float Sz_00;
};

// Kalman 상태값 초기화
void kalman_init(kalman *self, float dt, float Sz_00, float Sw_00, float Sw_11);

// Kalman 상태값 추정치 갱신 및 추정된 각도 반환
float kalman_update(kalman *self, float gyro_rate, float accel_angle);
