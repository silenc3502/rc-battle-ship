#include<stdio.h>
#include<math.h>

#define	M			3.01				// 질량			kg
#define	g			9.81			// 중력가속도	kg/m^2
#define	PI			3.1415926535897932384626433832795028841971693993751058209749445923078164062862089986280348253421170679	// pi 값
#define	Cthrust		0.0000116		// 추력계수
#define	Cdrag		0.000000068805		// 항력계수
#define	I_xx		0.0086		// 쿼드콥터 x의 관성모멘트	kg m^2
#define I_yy		0.0086		// 쿼드콥터 y의 관성모멘트	kg m^2
#define I_zz		0.0172		// 쿼드콥터 z의 관성모멘트	kg m^2
#define L			0.225			// 쿼드콥터의 한 쪽 길이	m
#define DegToRad		PI/180.			// c는 삼각함수에 라이안 값으로 집어 넣어야 한다.
#define R			0.128		//저항
/*
#define I_m			0.00003357		//rotor의 관성 모멘트
#define A_x			0.25
#define A_y			0.25
#define A_z			0.25
*/

typedef struct __Force
{
	double Fx;
	double Fy;
	double Fz;

}Force;

typedef struct __Moment
{
	double Mx;
	double My;
	double Mz;

}Moment;

typedef struct __Inertial
{
	double xDot;
	double yDot;
	double zDot;
	double x;
	double y;
	double z;
	double phiDot;
	double thetaDot;
	double psiDot;
	double psiDotSET;
	double psiSET;
	double phi;
	double theta;
	double psi;
	double phi_0;
	double theta_0;
	double psi_0;
	double Old_phi;
	double Old_theta;

	double Old_phiDot;
	double Old_thetaDot;
	double Old_psiDot;
	double Old_xDot; 
	double Old_yDot; 
	double Old_zDot;

}Inertial;

typedef struct __Body
{
	double uDot;
	double vDot;
	double wDot;
	double u;
	double v;
	double w;
	double pDot;
	double qDot;
	double rDot;
	double p;
	double q;
	double r;
	double Old_pDot;
	double Old_qDot;
	double Old_rDot;
	double Old_uDot;
	double Old_vDot;
	double Old_wDot;

}Body;

typedef struct __Omega
{
	double w1;
	double w2;
	double w3;
	double w4;
	double w1_sq;
	double w2_sq;
	double w3_sq;
	double w4_sq;

}Omega;

typedef struct __Input
{
	double T;
	double tau_phi;
	double tau_theta;
	double tau_psi;

}Input;

typedef struct __Sensor
{
	double Phi;
	double Theta;
	double Psi;
	double Phi_rad;
	double Theta_rad;
	double Psi_rad;

}Sensor;

void pid(float roll, float pitch);
//void model_init(Body _body, Inertial _inertial);
