#include <time.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "water_rocket_simulation.h"

// 표준 공기 밀도
#define	STANDARD_AIR_DENSITY		1.293
// 표준 대기압
#define STANDARD_ATMOSPHERE		101325
// 기체 상수
#define DRY_AIR_GAS_CONST		287.05
// 캘빈 상수
#define KELVIN_CONST			273.15
// 물의 밀도
#define WATER_DENSITY			1000.0
// kg/cm^2 -> Pa
#define KG_PER_CM_SQUARE_TO_PA		98066.5
// 1 atm -> 1 kg/cm^2
#define ATM_TO_KG_PER_CM_SQUARE		1.03323

float calc_pneumatic_density(float temper, float press)
{
	//return STANDARD_ATMOSPHERE / (DRY_AIR_GAS_CONST * (temper + KELVIN_CONST));
	return (press * STANDARD_ATMOSPHERE) / (DRY_AIR_GAS_CONST * (temper + KELVIN_CONST));
}

float calc_exhaust_velocity(float press)
{
	return sqrt((2 * (press - 1.0)) / WATER_DENSITY);
}

#if 0
float get_shr_with_convert_unit(float press)
{
	int i;
	float tmp, press_pa = press * STANDARD_ATMOSPHERE;

	for(i = 0; i < 11; i++)
	{
		if(press_pa > lashr_tbl[i][0] * KG_PER_CM_SQUARE_TO_PA &&
		   press_pa < lashr_tbl[i + 1][0] * KG_PER_CM_SQUARE_TO_PA)
		{
			float interval = lashr_tbl[i + 1][0] * KG_PER_CM_SQUARE_TO_PA - press_pa;
			tmp = (lashr_tbl[i][1] + (lashr_tbl[i + 1][1] - lashr_tbl[i][1])) *
			       KG_PER_CM_SQUARE_TO_PA * interval;
			return tmp;
		}
	}
}
#endif

float get_sat_shr_with_convert_unit(float press)
{
	int i;
	float tmp, press_kgcm2 = press * ATM_TO_KG_PER_CM_SQUARE;

	for(i = 0; i < 11; i++)
	{
		if(press_kgcm2 > sat_lashr_tbl[i][0] && press_kgcm2 < sat_lashr_tbl[i + 1][0])
		{
			float interval = sat_lashr_tbl[i + 1][0] - press_kgcm2;
			tmp = sat_lashr_tbl[i][1] + (sat_lashr_tbl[i + 1][1] - sat_lashr_tbl[i][1]) * interval;
			return tmp;
		}
	}
}

float get_shr_with_convert_unit(float temper, float press)
{
	int i;
	int p_idx, t_idx;

	float shr, shr_interval;
	float temper1, temper2;

	float p_error, t_error;
	float p_interval, t_interval;
	float tmp, press_kgcm2 = press * ATM_TO_KG_PER_CM_SQUARE;

	printf("press_kgcm2 = %f\n", press_kgcm2);

	for(i = 0; i < 13; i++)
	{
		if(press_kgcm2 > pressure[i] && press_kgcm2 < pressure[i + 1])
		{
			p_idx = i;
			p_error = press_kgcm2 - pressure[i];
			p_interval = pressure[i + 1] - pressure[i];
			break;
		}
		else
			p_idx = i;
	}

	printf("p_idx = %d, p_error = %f, p_interval = %f\n", p_idx, p_error, p_interval);

	for(i = 0; i < 51; i++)
	{
		if(temper > temperature[i] && temper < temperature[i + 1])
		{
			t_idx = i;
			t_error = temper - temperature[i];
			t_interval = temperature[i + 1] - temperature[i];
			break;
		}
		else
			t_idx = i;
	}

	printf("t_idx = %d, t_error = %f, t_interval = %f\n", t_idx, t_error, t_interval);
	printf("t_error / t_interval = %f, p_error / p_interval = %f\n", t_error / t_interval, p_error / p_interval);

	if(t_idx != 51)
	{
		printf("t_idx != 51\n");
		temper1 = lashr_tbl[t_idx][p_idx] + (1 + t_error / t_interval) * (lashr_tbl[t_idx + 1][p_idx] - lashr_tbl[t_idx][p_idx]);

		if(p_idx != 13)
			temper2 = lashr_tbl[t_idx][p_idx + 1] + (1 + t_error / t_interval) * (lashr_tbl[t_idx + 1][p_idx + 1] - lashr_tbl[t_idx][p_idx + 1]);
		else
			temper2 = temper1;
	}
	else
	{
		printf("t_idx == 51\n");
		temper1 = lashr_tbl[t_idx][p_idx];

		if(p_idx != 13)
			temper2 = lashr_tbl[t_idx][p_idx + 1];
		else
			temper2 = temper1;
	}

	printf("temper1 = %f, temper2 = %f\n", temper1, temper2);

	return temper1 + (temper2 - temper1) * p_error / p_interval;
}

/* Water Thrust 부분에서 쇼부 봐야함
   Air Thrust 까지 진행되면 자세 제어가 불가능함 */
int main(void)
{
	// F_R = m_R * d(v_R) / dt = F_Th + F_G + F_D
	// F_Th(t) = v_e(t) * d(m_R) / dt
	// p(t) + 0.5 * ρ_Water * v_in^2 = p_0 + ρ_Water * 0.5 * v_E^2

	// v_E(t) = sqrt([2 * (p(t) - p_0)]/ρ_Water)
	// p(0) * V_A^γ(0) = p(t) * V_A^γ(t) => p(t) = p(0) * (V_A(t) / V_A(0))^-γ
	// p(t) = p(0) * ([V_A(0) + (V_W(0) - V_W(t))]/ V_A(0))^-γ
	//      = p(0) * (1 + [m_W(0) - m_W(t)] / [ρ_Water * V_A(0)])^-γ
	// p(t) = p(0) * (1 + [m_R(0) - m_A(0) - m_0 - m_R(t) + m_A(t) + m_0] / [ρ_Water * V_A(0)])^-γ
	//      = p(0) * (1 + [m_R(0) - m_R(t)] / [ρ_Water * V_A(0)])^-γ
	// d(m_R) / dt = d(m_W) / dt = ρ_Water * d(V_water) / dt = ρ_Water * A_N * v_E(t)

	// v_E(t) = sqrt([2 * (p(t) - p_0)]/ρ_A)
	// p(t) = p(t_0) * (V_A(t) / V_A(t_0))^-γ
	//      = p(t_0) * ([V_R + V_(A, o)(t)] / V_R)^-γ
	//      = p(t_0) * (1 + [m_(A, o)(t)] / [ρ_A * V_R])^-γ
	//      = p(t_0) * (1 + [m_(A, i)(t_0) - m_(A, i)(t)] / [ρ_A * V_R])^-γ
	// m_(A, i)(t_0) = m_(A, i)(0) = V_A(0) * ρ'_A(p) = V_A(0) * p(0) / (R_S * T)
	// d[m_(A, i)] / dt = ρ_A * A_N * v_E(t)

	// F_G = m_R(t) * g

	// F_D = 0.5 * c_D * ρ_A * A * v_R^2
	// F_D = -0.5 * c_D * ρ_A * A(v_R.v_R) * (v_R / ||v_R||)

	/* result */
	// m_R * (dv_R / dt) = v_e(t) * (dm_R / dt) + m_R(t) * g - 0.5 * c_D * ρ_A * A * (v_R.v_R) * (v_R / ||v_R||)

	// v_E(t) = sqrt([2 * (p(t) - p_0)] / ρ_Water)
	// p(t) = p(0) * (1 + [m_W(0) - m_W(t)] / [ρ_Water * V_A(0)])^-γ
	// dm_R / dt = ρ_Water * A_N * v_E(t)

	// v_E(t) = sqrt([2 * (p(t) - p_0)] / ρ_A)
	// p(t) = p(t_0) * (1 + [m_(A, i)(t_0) - m_(A, i)(t)] / [ρ_A * V_R])^-γ
	// dm_(A, i) / dt = ρ_A * A_N * v_E(t)

	/* 여기서 m_R 은 전체 로켓에 대한 질량이다.
	   v_R 은 로켓의 속도에 해당한다.
	   v_e 는 노즐에서의 배기 속도에 해당한다.
	   c_D 는 항력 계수다.
	   ρ_A 는 공기 밀도에 해당한다.

	   먼저 공기 밀도를 구해보도록 한다.
	   온도값은 공기압 센서를 통해서 가져와야 하는데 우선 11 ~ 30 도 사이라고 가정하고 만든다. */
	float air_dens, temper, press;

	/* Exhaust Velocity(배출 속도) */
	float water_exhaust_vel;

	/* Saturation Specific Heat Ratio(포화 비열비) */
	//float sat_sh_ratio;

	/* Specific Heat Ratio(비열비) */
	float sh_ratio;

	/* Air Specific Heat Ratio(공기의 단열 계수 - 비열비) */
	float air_shr;

	/* Nozzle Area */
	float nozzle_AN;

	/* Derivative of Rocket Mass(시간에 따라 변화하는 로켓의 질량) */
	float dm_rocket;

	srand(time(NULL));
	// 섭씨 온도(10 ~ 30)
	temper = (rand() % 201 + 100) / 10.0;
	// 1 ~ 6 기압 사이에서 사용하는 0.01 기준의 스케일 팩터
	press = (rand() % 501 + 100) / 100.0;

	/* 먼저 보일 샤를의 법칙을 적용한다.
	   밀도 = p / RT

	   밀도 단위 = kg / m^3
	   압력 = Pa
	   기체 상수 = 287.05 J / kgK
	   온도 = 켈빈 온도
	   물의 밀도 = 1000 kg / m^3
	*/
	air_dens = calc_pneumatic_density(temper, press);
	printf("temper = %f, press = %f, air_dens = %f\n", temper, press, air_dens);

	/* 0.5 L 페트병에 0.24 ~ 0.26 L 의 물과 0.24 ~ 0.26 L 의 공기를 구성한다.
	   이때 공기를 6 기압으로 압축했다고 가정하고 문제를 해석해보도록 한다.
	   v_E(t) = sqrt([2 * (p(t) - p_0)]/ρ_Water) 를 보면
	   우리의 경우 위의 press 를 기압으로 활용하면 된다.
	   그리고 대기압은 1 로 물의 밀도와 함께 수식을 계산하면 된다. */
	water_exhaust_vel = calc_exhaust_velocity(press);
	printf("temper = %f, press = %f, water_exhaust_vel = %f\n", temper, press, water_exhaust_vel);

	/* 비열비를 구하지만 Saturation 정보이므로 잘못됨 */
#if 0
	sat_sh_ratio = get_sat_shr_with_convert_unit(press);
	printf("Gamma(specific heat ratio) = %f\n", sat_sh_ratio);
#endif

	/* 물에 대한 온도와 압력에 따른 적절한 비열비를 얻어온다 */
	sh_ratio = get_shr_with_convert_unit(temper, press);
	printf("sh_ratio = %f\n", sh_ratio);

	/* 물의 경우 온도가 많이 올라갔을 경우에만 단열 계수에 대한 차이가 벌어진다.
	   물로켓을 해석할 때 공기의 단열 계수는 1.4 로 취급하는 편이다. */
	air_shr = 1.4;

	/* Nozzle Area 값은 우리가 실제로 발사하는 로켓에 의해 고정된 상수값이다.
	   우리가 가진 시스템은 반지름이 2.5 cm 로 0.025 m 의 원 둘레를 생각하면 된다.
	   원의 둘레를 구하는 방법은 2 * pi * r 에 해당하므로 2 * 3.14xx * 0.025 가 된다. */
	nozzle_AN = 2 * M_PI * 0.025;
	printf("nozzle_AN = %f m\n", nozzle_AN);

	/* Nozzle Area 를 구했으므로 시간에 따라 변화하는 로켓의 질량을 산출해낼 수 있다.
	   물의 밀도 * 시간에 따라 변화하는 물의 부피는 아래의 형식과 같다.
	   물의 밀도 * Nozzle Area * 시간에 따라 변화하는 물 배출 속도 */
	dm_rocket = WATER_DENSITY * nozzle_AN * water_exhaust_vel;
	printf("dm_rocket = %f\n", dm_rocket);

	return 0;
}
