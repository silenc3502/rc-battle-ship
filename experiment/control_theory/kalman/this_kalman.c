#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "kalman.h"

#define SAMPLE_COUNT 512
#define PI 3.1415926535897932384626433832795028841971693993751058209749445923078164062862089986280348253421170679

#define n 2
#define m 1
#define r 1

kalman roll_kalman_state;
kalman pitch_kalman_state;

// 행렬 곱셈 (A x B) = m by n 행렬
static void matrix_multiply(float* A, float* B, int a, int p, int b, float* C)
{
	// A = 입력 행렬 (m x p)
	// B = 입력 행렬 (p x n)
	// m = A 행렬의 행 갯수
	// p = A 행렬의 열 갯수 = B 행렬의 행 갯수
	// n = B 행렬의 열 갯수
	// C = 출력 행렬 = A * B (m x n)

	int i, j, k;
	for (i = 0; i<a; i++)
		for (j = 0; j<b; j++)
		{
			C[b*i + j] = 0;
			for (k = 0; k<p; k++)
				C[b*i + j] = C[b*i + j] + A[p*i + k] * B[b*k + j];
		}
}

// 행렬 덧셈 (A + B)
static void matrix_addition(float* A, float* B, int a, int b, float* C)
{
	// A = 입력 행렬 (m x n)
	// B = 입력 행렬 (m x n)
	// m = A 행렬의 행의 갯수 = B 행렬의 행의 갯수
	// n = A 행렬의 열의 갯수 = B 행렬의 열의 갯수
	// C = 출력 행렬 = A + B (m x n)

	int i, j;
	for (i = 0; i<a; i++)
		for (j = 0; j<b; j++)
			C[b*i + j] = A[b*i + j] + B[b*i + j];
}

// 행렬 뺄셈 (A - B) 
static void matrix_subtraction(float* A, float* B, int a, int b, float* C)
{
	// A = 입력 행렬 (m x n)
	// B = 입력 행렬 (m x n)
	// m = A 행렬의 행의 갯수 = B 행렬의 행의 갯수
	// n = A 행렬의 열의 갯수 = B 행렬의 열의 갯수
	// C = 출력 행렬 = A - B (m x n)

	int i, j;
	for (i = 0; i<a; i++)
		for (j = 0; j<b; j++)
			C[b*i + j] = A[b*i + j] - B[b*i + j];
}

// 전치 행렬 만들기
static void matrix_transpose(float* A, int a, int b, float* C)
{
	// A = 입력 행렬 (m x n)
	// m = A 행렬의 행 갯수
	// n = A 행렬의 열 갯수
	// C = 출력 행렬 = A 행렬의 전치 (n x m)
	int i, j;
	for (i = 0; i<a; i++)
		for (j = 0; j<b; j++)
			C[a*j + i] = A[b*i + j];
}

// 역행렬 구하기
static int matrix_inversion(float* A, int b, float* AInverse)
{
	// A = 입력 행렬 (n x n)
	// n = A 의 차원
	// AInverse = 역행렬 (n x n)
	// 수업 시간에 학습한 가우스 조르단 기법으로 구현함(성공시 1, 실패시 0)
	int i, j, iPass, imx, icol, irow;
	float det, temp, pivot, factor;
	float* ac = (float*)calloc(b*b, sizeof(float));
	det = 1;
	for (i = 0; i < b; i++)
	{
		for (j = 0; j < b; j++)
		{
			AInverse[b*i + j] = 0;
			ac[b*i + j] = A[b*i + j];
		}
		AInverse[b*i + i] = 1;
	}

	// 현재 행의 중심점은 iPass 이며 각 pass 에 대해 먼저 열의 중심점에서 최대 요소를 찾는다.
	for (iPass = 0; iPass < b; iPass++)
	{
		imx = iPass;
		for (irow = iPass; irow < b; irow++)
		{
			if (fabs(A[b*irow + iPass]) > fabs(A[b*imx + iPass]))
				imx = irow;
		}

		// A 와 AInverse 에서 모두 행 iPass 와 행 imx 의 요소를 교환한다.
		if (imx != iPass)
		{
			for (icol = 0; icol < b; icol++)
			{
				temp = AInverse[b*iPass + icol];
				AInverse[b*iPass + icol] = AInverse[b*imx + icol];
				AInverse[b*imx + icol] = temp;
				if (icol >= iPass)
				{
					temp = A[b*iPass + icol];
					A[b*iPass + icol] = A[b*imx + icol];
					A[b*imx + icol] = temp;
				}
			}
		}

		// 현재 중심축이 A[iPass][iPass]
		// 행렬식을 구한다.
		pivot = A[b*iPass + iPass];
		det = det * pivot;
		if (det == 0)
		{
			free(ac);
			return 0;
		}

		for (icol = 0; icol < b; icol++)
		{
			// 표준화한다.
			AInverse[b*iPass + icol] = AInverse[b*iPass + icol] / pivot;
			if (icol >= iPass) A[b*iPass + icol] = A[b*iPass + icol] / pivot;
		}

		for (irow = 0; irow < b; irow++)
		{
			// 가우스-조르단 문제와 동일하게 관계 없는 항을 밀어버린다.
			if (irow != iPass) factor = A[b*irow + iPass];
			for (icol = 0; icol < b; icol++)
			{
				if (irow != iPass)
				{
					AInverse[b*irow + icol] -= factor * AInverse[b*iPass + icol];
					A[b*irow + icol] -= factor * A[b*iPass + icol];
				}
			}
		}
	}

	free(ac);
	return 1;
}

// Kalman 상태값 초기화
void kalman_init(kalman *self, float dt, float Sz_00, float Sw_00, float Sw_11)
{
	// 출력 초기화
	self->xhat[0][0] = 0.0;
	self->xhat[1][0] = 0.0;

	self->P[0][0] = 3.0;
	self->P[0][1] = 0.0;
	self->P[1][0] = 0.0;
	self->P[1][1] = 3.0;

	self->A[0][0] = 1.0;
	self->A[0][1] = -dt;
	self->A[1][0] = 0.0;
	self->A[1][1] = 1.0;

	self->B[0][0] = dt;
	self->B[1][0] = 0.0;

	self->C[0][0] = 1.0;
	self->C[1][0] = 0.0;
	self->Sz[0][0] = Sz_00;

	self->Sw[0][0] = Sw_00;
	self->Sw[0][1] = 0.0;
	self->Sw[1][0] = 0.0;
	self->Sw[1][1] = Sw_11;
}

float kalman_update(kalman *self,  float gyroscope_rate,  float accelerometer_angle)
{
																		// Inputs.
	float u[m][m];              // Gyroscope rate.	자이로 각속도 deg/s
	float y[m][m];              // Accelerometer angle.	가속도 각도 deg

	u[0][0] = gyroscope_rate;
	y[0][0] = accelerometer_angle;

	// 임시값들
	float AP[n][n];             
	float CT[n][r];       
	float PCT[n][r];
	float CP[r][n];
	float CPCT[r][r]; 
	float CPCTSz[r][r];
	float CPCTSzInv[r][r];  
	float K[n][r];     
	float Cxhat[r][1]; 
	float yCxhat[r][1]; 
	float KyCxhat[n][1];    
	float Axhat[n][1];   
	float Bu[n][1];   
	float AxhatBu[n][1];  
	float AT[n][n];
	float APAT[n][n];
	float KC[n][n];
	float KCP[n][n];

	/* 
	자이로 input 으로 추론한 추정치에 의해 상태추정치를 업데이트
	xhat_est = A * xhat + B * u
	xhat_est = AxhatBu -> 현재 추정치, A = 시스템모델, B = ???, u = 자이로 각속도
	*/
	
	matrix_multiply((float*)self->A, (float*)self->xhat, n, n, 1, (float*)Axhat);
	matrix_multiply((float*)self->B, (float*)u, n, r, 1, (float*)Bu);
	matrix_addition((float*)Axhat, (float*)Bu, n, 1, (float*)AxhatBu);

	/* 추정 오차의 새로운 공분산을 계산한다.
		 P = A * P * A' - K * C * P * A' + Sw */
	matrix_transpose((float*)self->A, n, n, (float*)AT);
	matrix_multiply((float*)self->A, (float*)self->P, n, n, n, (float*)AP);
	matrix_multiply((float*)AP, (float*)AT, n, n, n, (float*)APAT);
	matrix_addition((float*)APAT, (float*)self->Sw, n, n, (float*)self->P);
		
	/* 
	s = 2.0C * P * C' + Sz
	칼만 이득 계산하기전
	H *P(k)*H(T) + R
	H = C, P = 공분산, Sz = R
	*/
	matrix_transpose((float*)self->C, r, n, (float*)CT);
	matrix_multiply((float*)self->C, (float*)self->P, r, n, n, (float*)CP);
	matrix_multiply((float*)CP, (float*)CT, r, n, r, (float*)CPCT);
	matrix_addition((float*)CPCT, (float*)self->Sz, r, r, (float*)CPCTSz);
	
	/*	
	칼만이득 계산
	K = A * P * C' * inv(s)
	선형 칼만식에 칼만 이득은 K(k) = P'(k)*H(T)*(H*P'(k)*H(T) + R)inv 임
	*/

	matrix_multiply((float*)self->P, (float*)CT, n, n, r, (float*)PCT);
	matrix_inversion((float*)CPCTSz, r, (float*)CPCTSzInv);
	matrix_multiply((float*)PCT, (float*)CPCTSzInv, n, r, r, (float*)K);

//	printf("%f  %f ", K[0][0],K[1][0]);
	

	/*  
	Inn = y - c * xhat;
	C = H(시스템모델)
	yCxhat(Inn) = z(k)(y -> 가속도 각도값) - H*x'^(k) -> z(k)를 통해 추정치 업데이트
	*/
	matrix_multiply((float*)self->C, (float*)AxhatBu, r, n, 1, (float*)Cxhat);
	matrix_subtraction((float*)y, (float*)Cxhat, r, 1, (float*)yCxhat);

	/*
	z(k)값을 통해 추정치 업데이트
	여기서 z(k)는 가속도 각도값
	xhat = xhat_est + K * Inn;
	x^(k) = {x'^(k) = xhat_est} + [{K(k)*(z(k) - H*x'^(k))} = Inn]
	*/
	matrix_multiply((float*)K, (float*)yCxhat, n, r, 1, (float*)KyCxhat);
	matrix_addition((float*)AxhatBu, (float*)KyCxhat, n, 1, (float*)self->xhat);
	
	/*
	에러공분산 업데이트
	P(k) = (I-K(k)H)P'(k)
	*/

	matrix_multiply((float*)K, (float*)self->C, n, r, n, (float*)KC);
	matrix_multiply((float*)KC, (float*)self->P, n, n, n, (float*)KCP);
	matrix_subtraction((float*)self->P, (float*)KCP, n, n, (float*)self->P);
//	printf("P_00 = %f P_01 = %f P_10 = %f P_11 = %f \n", self->P[0][0], self->P[0][1], self->P[1][0], self->P[1][1]);
	// 추정치 반환
	return self->xhat[0][0];
}
