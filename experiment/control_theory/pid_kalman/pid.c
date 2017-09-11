#include "kalman_variable.h"
#include "kalman.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

void pid(float rolla, float pitcha)
{
	Input _input;
	Force _force;			//쿼드콥터 힘
	Moment _moment;			//쿼드콥터 모멘트
	Body _body;				//기체좌표계 변수
	Inertial _inertial;		//관성좌표계 변수
	Omega _omega;			//각속도 모터에 속도
	Sensor _sensor;			//센서에서 받는 값

	double enterT = 0, enterTauPhi = 0, enterTauTheta = 0, enterTauPsi = 0;		//입력될 T
	double height;
	double degree_rad;			//원하는 변경 각도
	double dt = 0.0025;		//성능에 따라 적분을 다르게 해주어야함

	int cnt = 0;			//몇 번만에 원하는 각으로 가는지 체크
	int select;				//호버링,전후좌우 모드 설정
	int flag_sensor = 1;		//센서 On
	int flag_Theta, flag_Psi, flag_Phi;
	int flag_init = 1;		//Roll Pitch 다 잡은후 Psi 잡을 때 초기화 사용

	double Pwm1, Pwm2, Pwm3, Pwm4;
	double Volt1, Volt2, Volt3, Volt4;

	double pst;				//소요시간 확인 용
	clock_t start, end;		//소요시간 확인 용

	double KpP = 2.5, KdP = 0.22;
	double KpR = 2.5, KdR = 0.22;
	double KpY = 2.5, KdY = 0.455;


	double errP = 0;
	double KpP_term = 0;
	double D_errP = 0;
	double prev_errP = 0;
	double KdP_term = 0;
	double errR = 0;
	double KpR_term = 0;
	double D_errR = 0;
	double prev_errR = 0;
	double KdR_term = 0;
	double errY = 0;
	double KpY_term = 0;
	double D_errY = 0;
	double prev_errY = 0;
	double KdY_term = 0;

	


	_sensor.Phi = 0.1;
	_sensor.Theta = -0.1;
	_sensor.Psi = 0.1;

	_body.p = 0;
	_body.q = 0;
	_body.r = 0;
	_body.pDot = 0;
	_body.qDot = 0;
	_body.rDot = 0;
	_body.v = 0;
	_body.u = 0;
	_body.w = 0;
	_body.vDot = 0;
	_body.uDot = 0;
	_body.wDot = 0;
	_inertial.phi = 0;
	_inertial.theta = 0;
	_inertial.psi = 0;
	_inertial.phiDot = 0;
	_inertial.thetaDot = 0;
	_inertial.psiDot = 0;
	_inertial.x = 0;
	_inertial.y = 0;
	_inertial.z = 0;
	_inertial.xDot = 0;
	_inertial.yDot = 0;
	_inertial.zDot = 0;
	_inertial.Old_phiDot = 0;
	_inertial.Old_psiDot = 0;
	_inertial.Old_thetaDot = 0;
	_body.Old_pDot = 0;
	_body.Old_qDot = 0;
	_body.Old_rDot = 0;
	_inertial.Old_xDot = 0;
	_inertial.Old_yDot = 0;
	_inertial.Old_zDot = 0;
	_body.Old_uDot = 0;
	_body.Old_vDot = 0;
	_body.Old_wDot = 0;
	//model_init(_body, _inertial);
	//getchar();
	/*printf("움직일 방향을 입력하세요\n");
	printf("0.호버링\n1.전후\n2.좌우\n3.상승하강\n입력 : ");
	printf("0 Hovering / 1 전 / 2 후 / 3 좌 / 4 우 / 5 상 / 6 하\n");
	scanf("%d", &select);*/
	select = 0;
	switch (select)
	{
	case 0:
		enterT = 0;
		break;
	case 1:
		degree_rad = 20 * PI / 180;
		enterTauTheta = 20 * PI / 180;
		flag_sensor = 0;
		break;
	case 2:
		degree_rad = -20 * PI / 180;
		enterTauTheta = -20 * PI / 180;
		flag_sensor = 0;
		break;
	case 3:
		degree_rad = 20 * PI / 180;
		enterTauPhi = 20 * PI / 180;
		flag_sensor = 0;
		break;
	case 4:
		degree_rad = -20 * PI / 180;
		enterTauPhi = -20 * PI / 180;
		flag_sensor = 0;
		break;
	case 5:
		enterT = 0.1;
		flag_sensor = 0;
	case 6:
		enterT = -0.1;
		flag_sensor = 0;
	default:
		break;
	}
	

	start = clock();
	while (1)
	{
		////////////////// 입력 //////////////////
		cnt++;
		// input 힘 단위 //
		_input.T = M*g + enterT;
		_input.tau_phi = enterTauPhi;
		_input.tau_theta = enterTauTheta;
		_input.tau_psi = enterTauPsi;

		// omega^2 값 //

		_omega.w1_sq = (_input.T / (4 * Cthrust)) - (_input.tau_phi / (2 * L * sin(45 * DegToRad))) + (_input.tau_theta / (2 * L * sin(45 * DegToRad))) + (_input.tau_psi / (4 * Cthrust));
		_omega.w2_sq = (_input.T / (4 * Cthrust)) - (_input.tau_phi / (2 * L * sin(45 * DegToRad))) - (_input.tau_theta / (2 * L * sin(45 * DegToRad))) - (_input.tau_psi / (4 * Cthrust));
		_omega.w3_sq = (_input.T / (4 * Cthrust)) + (_input.tau_phi / (2 * L * sin(45 * DegToRad))) - (_input.tau_theta / (2 * L * sin(45 * DegToRad))) + (_input.tau_psi / (4 * Cthrust));
		_omega.w4_sq = (_input.T / (4 * Cthrust)) + (_input.tau_phi / (2 * L * sin(45 * DegToRad))) + (_input.tau_theta / (2 * L * sin(45 * DegToRad))) - (_input.tau_psi / (4 * Cthrust));

		// omega 값 //	
		_omega.w1 = sqrt(_omega.w1_sq);		//w1
		_omega.w2 = sqrt(_omega.w2_sq);		//w2
		_omega.w3 = sqrt(_omega.w3_sq);		//w3
		_omega.w4 = sqrt(_omega.w4_sq);		//w4

		if (_omega.w1>1050)
			_omega.w1 = 1050;
		if (_omega.w4>1050)
			_omega.w4 = 1050;
		if (_omega.w2>1050)
			_omega.w2 = 1050;
		if (_omega.w3>1050)
			_omega.w3 = 1050;

		////////////////// PWM //////////////////
		Volt1 = _omega.w1 * 60 / (2 * PI * 690) + R / (60 / (2 * PI * 690))*Cdrag*_omega.w1*_omega.w1; //RPM/Kv = omega*60/(2*PI*Kv)
		Volt2 = _omega.w2 * 60 / (2 * PI * 690) + R / (60 / (2 * PI * 690))*Cdrag*_omega.w2*_omega.w2; //RPM/Kv = omega*60/(2*PI*Kv)
		Volt3 = _omega.w3 * 60 / (2 * PI * 690) + R / (60 / (2 * PI * 690))*Cdrag*_omega.w3*_omega.w3; //RPM/Kv = omega*60/(2*PI*Kv)
		Volt4 = _omega.w4 * 60 / (2 * PI * 690) + R / (60 / (2 * PI * 690))*Cdrag*_omega.w4*_omega.w4; //RPM/Kv = omega*60/(2*PI*Kv)
		

		Pwm1 = Volt1 / 14.8;
		Pwm2 = Volt2 / 14.8;
		Pwm3 = Volt3 / 14.8;
		Pwm4 = Volt4 / 14.8;
	
		////////////////// Force & Moment //////////////////
		// Force //
		_force.Fx = 0;
		_force.Fy = 0;
		_force.Fz = (_omega.w1_sq + _omega.w2_sq + _omega.w3_sq + _omega.w4_sq);

		// Moment //
		_moment.Mx = L *sin(45 * DegToRad)*(-(_omega.w1_sq + _omega.w2_sq) + _omega.w3_sq + _omega.w4_sq);
		_moment.My = L *sin(45 * DegToRad)*(_omega.w1_sq - _omega.w2_sq - _omega.w3_sq + _omega.w4_sq);
		_moment.Mz = Cthrust * (_omega.w1_sq - _omega.w2_sq + _omega.w3_sq - _omega.w4_sq);

		////////////////// Angular Velocity & Angle //////////////////
		////////////////// phi, theta, psi, p, q, r //////////////////
		////////////////// phiDot, thetaDot, psiDot, pDot, qDot, rDot //////////////////

		// Body coordinate _ angluar//
		_body.pDot = (_body.q*_body.r*(I_yy - I_zz) + _moment.Mx) / I_xx;
		_body.qDot = (_body.p*_body.r*(I_zz - I_xx) + _moment.My) / I_yy;
		_body.rDot = _moment.Mz / I_zz;

		/*
		printf("============================== pDot, qDot, rDot ==============================\n");
		*/
		// Body angular Velocity Intergral _ make p,q,r //
		if (_body.pDot >_body.Old_pDot)
			_body.p = _body.p + _body.Old_pDot*dt + (_body.pDot - _body.Old_pDot)*dt / 2;
		else
			_body.p = _body.p + _body.Old_pDot*dt - (_body.Old_pDot - _body.pDot)*dt / 2;

		if (_body.qDot >_body.Old_qDot)
			_body.q = _body.q + _body.Old_qDot*dt + (_body.qDot - _body.Old_qDot)*dt / 2;
		else
			_body.q = _body.q + _body.Old_qDot*dt - (_body.Old_qDot - _body.qDot)*dt / 2;

		if (_body.rDot >_body.Old_rDot)
			_body.r = _body.r + _body.Old_rDot*dt + (_body.rDot - _body.Old_rDot)*dt / 2;
		else
			_body.r = _body.r + _body.Old_rDot*dt - (_body.Old_rDot - _body.rDot)*dt / 2;

		_body.Old_pDot = _body.pDot;
		_body.Old_qDot = _body.qDot;
		_body.Old_rDot = _body.rDot;

		// 관성 좌표계 //
		_inertial.phiDot = (
			cos(_inertial.psi) / cos(_inertial.theta) * _body.p +
			sin(_inertial.psi) / cos(_inertial.theta) * _body.q);

		_inertial.thetaDot = (
			-sin(_inertial.psi) * _body.p +
			cos(_inertial.psi) * _body.q);

		_inertial.psiDot = (
			cos(_inertial.psi)*tan(_inertial.theta) * _body.p +
			sin(_inertial.psi)*tan(_inertial.theta) * _body.q +
			_body.r);

		// Inertial Angular Velocity Intergral _ make angle phi, theta, psi //	

		if (_inertial.phiDot > _inertial.Old_phiDot)
			_inertial.phi = _inertial.phi + _inertial.Old_phiDot*dt + (_inertial.phiDot - _inertial.Old_phiDot)*dt / 2;
		else
			_inertial.phi = _inertial.phi + _inertial.Old_phiDot*dt - (_inertial.Old_phiDot - _inertial.phiDot)*dt / 2;

		if (_inertial.thetaDot > _inertial.Old_thetaDot)
			_inertial.theta = _inertial.theta + _inertial.Old_thetaDot*dt + (_inertial.thetaDot - _inertial.Old_thetaDot)*dt / 2;
		else
			_inertial.theta = _inertial.theta + _inertial.Old_thetaDot*dt - (_inertial.Old_thetaDot - _inertial.thetaDot)*dt / 2;

		if (_inertial.psiDot > _inertial.Old_psiDot)
			_inertial.psi = _inertial.psi + _inertial.Old_psiDot*dt + (_inertial.psiDot - _inertial.Old_psiDot)*dt / 2;
		else
			_inertial.psi = _inertial.psi + _inertial.Old_psiDot*dt - (_inertial.Old_psiDot - _inertial.psiDot)*dt / 2;

		_inertial.Old_phiDot = _inertial.phiDot;
		_inertial.Old_thetaDot = _inertial.thetaDot;
		_inertial.Old_psiDot = _inertial.psiDot;

		////////////////    Velocity and Position    ////////////////
		/* x, y, z, u, v, w,
		   xDot, yDot, zDpt, uDot, vDot, wDot */

		// 물체 좌표계 _ accerlation //
		_body.uDot = g*sin(_inertial.theta) + (_body.r*_body.v - _body.q*_body.w);
		_body.vDot = -g*sin(_inertial.phi)*cos(_inertial.theta) + (_body.p*_body.w - _body.r*_body.u);
		_body.wDot = _force.Fz / M - g*cos(_inertial.theta)*cos(_inertial.theta) + (_body.q*_body.u - _body.p*_body.v);

		// 몸체 속도 적분 _ make u,v,w //
		if (_body.uDot >_body.Old_uDot)
			_body.u = _body.u + _body.Old_uDot*dt + (_body.uDot - _body.Old_uDot)*dt / 2;
		else
			_body.u = _body.u + _body.Old_uDot*dt - (_body.Old_uDot - _body.uDot)*dt / 2;

		if (_body.vDot >_body.Old_vDot)
			_body.v = _body.v + _body.Old_vDot*dt + (_body.vDot - _body.Old_vDot)*dt / 2;
		else
			_body.v = _body.v + _body.Old_vDot*dt - (_body.Old_vDot - _body.vDot)*dt / 2;

		if (_body.wDot >_body.Old_wDot)
			_body.w = _body.w + _body.Old_wDot*dt + (_body.wDot - _body.Old_wDot)*dt / 2;
		else
			_body.w = _body.w + _body.Old_wDot*dt - (_body.Old_wDot - _body.wDot)*dt / 2;

		_body.Old_uDot = _body.uDot;
		_body.Old_vDot = _body.vDot;
		_body.Old_wDot = _body.wDot;

		// 관성 좌표계 //
		_inertial.xDot = (
			cos(_inertial.psi)*cos(_inertial.theta)*_body.u +
			(cos(_inertial.psi)*sin(_inertial.theta)*sin(_inertial.phi) - sin(_inertial.psi)*cos(_inertial.phi))*_body.v +
			(cos(_inertial.psi)*sin(_inertial.theta)*cos(_inertial.phi) + sin(_inertial.psi)*sin(_inertial.phi))*_body.w
			);

		_inertial.yDot = (
			sin(_inertial.psi)*cos(_inertial.theta)*_body.u +
			(sin(_inertial.psi)*sin(_inertial.theta)*sin(_inertial.phi) + cos(_inertial.psi)*cos(_inertial.phi))*_body.v +
			(sin(_inertial.psi)*sin(_inertial.theta)*cos(_inertial.phi) - cos(_inertial.psi)*sin(_inertial.phi))*_body.w
			);

		_inertial.zDot = (
			-sin(_inertial.theta)*_body.u +
			cos(_inertial.theta)*sin(_inertial.psi)*_body.v +
			cos(_inertial.theta)*cos(_inertial.psi)*_body.w
			);

		// 관성 속도 적분 _ make position x,y,z //	
		if (_inertial.xDot > _inertial.Old_xDot)
			_inertial.x = _inertial.x + _inertial.Old_xDot*dt + (_inertial.xDot - _inertial.Old_xDot)*dt / 2;
		else
			_inertial.x = _inertial.x + _inertial.Old_xDot*dt - (_inertial.Old_xDot - _inertial.xDot)*dt / 2;

		if (_inertial.yDot > _inertial.Old_yDot)
			_inertial.y = _inertial.y + _inertial.Old_yDot*dt + (_inertial.yDot - _inertial.Old_yDot)*dt / 2;
		else
			_inertial.y = _inertial.y + _inertial.Old_yDot*dt - (_inertial.Old_yDot - _inertial.yDot)*dt / 2;

		if (_inertial.zDot > _inertial.Old_zDot)
			_inertial.z = _inertial.z + _inertial.Old_zDot*dt + (_inertial.zDot - _inertial.Old_zDot)*dt / 2;
		else
			_inertial.z = _inertial.z + _inertial.Old_zDot*dt - (_inertial.Old_zDot - _inertial.zDot)*dt / 2;

		_inertial.Old_xDot = _inertial.xDot;
		_inertial.Old_yDot = _inertial.yDot;
		_inertial.Old_zDot = _inertial.zDot;
		////////////////// Sensor Check //////////////////

		if (flag_sensor)
		{
			while (_sensor.Phi == 0 && _sensor.Theta == 0 && _sensor.Psi == 0);
			//센서는 degree 값으로 받는다.


			_sensor.Phi = rolla;//-10;
			flag_Phi = 1;
			printf("sensor.phi = %lf\n", _sensor.Phi);
			_sensor.Theta = pitcha;//10;
			flag_Theta = 1;
			printf("sensor.theta = %lf\n",_sensor.Theta);
			_sensor.Psi = 0;
			flag_Psi = 0;

			if (abs(_sensor.Psi) >= 180)
			{
				if (_sensor.Psi >180)
					_sensor.Psi = _sensor.Psi - 360;
				else if (_sensor.Psi <= -180)
					_sensor.Psi = _sensor.Psi + 360;
			}

			_sensor.Phi_rad = _sensor.Phi*PI / 180;
			_sensor.Theta_rad = _sensor.Theta*PI / 180;
			_sensor.Psi_rad = _sensor.Psi*PI / 180;
			flag_sensor = 0;		//센서 OFF
		}

		// stop condition //
		if (select == 0)
		{
			if (flag_Theta == 1 || flag_Phi == 1 && flag_Psi == 0)
			{
				if ((_inertial.theta + _sensor.Theta_rad) * 180 / PI < 0.01 && (_inertial.theta + _sensor.Theta_rad) * 180 / PI > -0.01)
				{
					flag_Theta = 0;	//세팅 완료
					enterTauTheta = 0;
				}
				else if (flag_Psi == 0)
					flag_Theta = 1;

				if ((_inertial.phi + _sensor.Phi_rad) * 180 / PI < 0.01 && (_inertial.phi + _sensor.Phi_rad) * 180 / PI > -0.01)
				{
					flag_Phi = 0;		//세팅 완료
					enterTauPhi = 0;
				}
				else if (flag_Psi == 0)
					flag_Phi = 1;

				if (flag_Theta)
				{
					errP = _inertial.theta + _sensor.Theta_rad;
					KpP_term = KpP * errP;

					D_errP = (errP - prev_errP) / dt;
					KdP_term = KdP * D_errP;

					prev_errP = errP;
					enterTauTheta = -(KpP_term + KdP_term);
				
				}
				if (flag_Phi)
				{
					errR = _inertial.phi + _sensor.Phi_rad;
					KpR_term = KpR * errR;

					D_errR = (errR - prev_errR) / dt;
					KdR_term = KdR * D_errR;

					prev_errR = errR;
					enterTauPhi = -(KpR_term + KdR_term);
				
				}
				
			}
			else if (flag_Theta == 0 && flag_Phi == 0)
			{	
				if (flag_init == 1)
				{
					// Roll 과 Pitch 를 잡은후 Yaw 를 설정함
					// 따라서 기존 Roll, Pitch 를 다 잡았다는 가정 후 설정 초기화 
					enterTauTheta = 0;
					enterTauPhi = 0;
					_inertial.Old_phi = _inertial.phi;
					_inertial.Old_theta = _inertial.theta;
					_body.p = 0;
					_body.q = 0;
					_body.r = 0;
					_body.pDot = 0;
					_body.qDot = 0;
					_body.rDot = 0;
					_inertial.phi = 0;
					_inertial.theta = 0;
					_inertial.phiDot = 0;
					_inertial.thetaDot = 0;
					flag_init = 0;
				}
				else if ((_inertial.psi + _sensor.Psi_rad) * 180 / PI < 0.1 && (_inertial.psi + _sensor.Psi_rad) * 180 / PI > -0.1)//hovering 조건
				{
					_inertial.phi += _inertial.Old_phi;
					_inertial.theta += _inertial.Old_theta;
					flag_Psi = 0;
					flag_sensor = 1;		//센서 On
					flag_init = 1;
					enterTauPsi = 0;
					break;
				}
				else
				{
					flag_Psi = 1;
		
					errY = _inertial.psi + _sensor.Psi_rad;
					KpY_term = KpY* errY;

					D_errY = (errY - prev_errY) / 0.01;
					KdY_term = KdY * D_errY;

					prev_errY = errY;
					enterTauPsi = -KpY_term;
				}
			}
			
		}
		else if (select == 1 || select == 2)
		{
			if ((_inertial.theta + degree_rad) * 180 / PI  < 0.02 && (_inertial.theta + degree_rad) * 180 / PI  > -0.02)
			{
				printf("theta ok\n");
				flag_sensor = 1;		//센서 On
				break;
			}
			else {
				errP = _inertial.theta + degree_rad;
				KpP_term = KpP * errP;

				D_errP = (errP - prev_errP) / dt;
				KdP_term = KdP * D_errP;

				prev_errP = errP;
				enterTauTheta = -(KpP_term + KdP_term);
		
			}
		}
		else if (select == 3 || select == 4)
		{
			if ((_inertial.phi + degree_rad) * 180 / PI < 0.02 && (_inertial.phi + degree_rad) * 180 / PI > -0.02)
			{
				printf("phi ok\n");
				flag_sensor = 1;		//센서 On
				break;
			}
			else {
				errR = _inertial.phi + degree_rad;
				KpR_term = KpR * errR;

				D_errR = (errR - prev_errR) / dt;
				KdR_term = KdR * D_errR;

				prev_errR = errR;
				enterTauPhi = -(KpR_term + KdR_term);
			
			}
		}
		else if (select == 5)
		{
			if (_omega.w3 >= 800)
			{
				printf("phi ok\n");
				flag_sensor = 1;		//센서 On
				break;
			}
			else
				enterT += 0.1;
		}
		else if (select == 6)
		{
			if (_omega.w3 <= 200)
			{
				printf("phi ok\n");
				flag_sensor = 1;		//센서 On
				break;
			}
			else
				enterT += -0.1;
		}
		

		printf("phi : %lf ", (_inertial.phi) * 180 / PI);
		printf("theta : %lf ", (_inertial.theta) * 180 / PI);
		printf("psi : %lf	/ ", (_inertial.psi) * 180 / PI);
		printf("w1 : %lf w2 : %lf w3 : %lf w4 : %lf \n",_omega.w1, _omega.w2, _omega.w3, _omega.w4);
		
	}
	end = clock();
	pst = (double)(end - start) / CLOCKS_PER_SEC;
	printf("걸린 시간은 %lf 입니다.\n", pst);
	printf("final cnt : %d\n", cnt);
	printf("phi- : %lf\n", (_inertial.phi) * 180 / PI);
	printf("theta- : %lf\n", (_inertial.theta) * 180 / PI);
	printf("psi- : %lf\n", (_inertial.psi) * 180 / PI);
//	printf("body_phi : %lf body_theta : %lf body_psi : %lf \n", _input.tau_phi, _input.tau_theta, _input.tau_psi);

	//getchar(); getchar();
	//return 0;
}
