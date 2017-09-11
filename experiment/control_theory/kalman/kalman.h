typedef struct _kalman kalman;

struct _kalman
{
	float A[2][2] ;	/* 시스템 모델 A, 현 단계에서의 상태변수와 다음단계의
			   상태변수를 연결하는 변환계수 */
	float B[2][1] ;
	float C[2][1] ;		// 시스템모델 H time step에 따른 변화 가능성
	float Sz[1][1] ;	// 시스템모델 R 측정 노이즈 분산
	float Sw[2][2];		// 시스템모델 Q 프로그램 노이즈 분산

	// 영구적 상태값
	float xhat[2][1] ;	// 초기 상태변수값 아마 자이로 가속도
	float P[2][2] ;		// 초기 오차 공분산

};

void kalman_init(kalman *self, float dt, float Sz_00, float Sw_00, float Sw_11);

float kalman_update(kalman *self, float gyro_rate, float accel_angle);
