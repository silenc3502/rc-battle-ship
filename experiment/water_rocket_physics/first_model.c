#include <stdio.h>
#include <math.h>

#define WATER_DENSITY		998
#define DRAG_COEFFICIENT	0.98
#define GRAVITY			9.8
#define ROCKET_DRAG		0.85

#define ATM_TO_PSI		14.695948775
#define PSI_TO_N_M2		6894.757293

/* 로켓 발사 파라메터 1 */
#define ANGLE1			45.0
#define INIT_PRESSURE_ATM1	2.72184
#define WATER_VOLUME1		0.7
#define TOTAL_VOLUME1		2.0
#define INIT_AIR_VOLUME1	1.3
#define FINAL_AIR_VOLUME1	2.0
#define ROCKET_MASS1		0.3
#define NOZZLE_RADIUS1		0.0105

/* 로켓 발사 파라메터 2 */
#define ANGLE2			30.0
#define INIT_PRESSURE_ATM2	2.0
#define WATER_VOLUME2		0.25
#define TOTAL_VOLUME2		0.5
#define INIT_AIR_VOLUME2	0.25
#define FINAL_AIR_VOLUME2	0.5
#define ROCKET_MASS2		0.0034
#define NOZZLE_RADIUS2		0.012

/* 로켓 발사 파라메터 3 */
#define ANGLE3			30.0
#define INIT_PRESSURE_ATM3	2.0
#define WATER_VOLUME3		0.167
#define TOTAL_VOLUME3		0.5
#define INIT_AIR_VOLUME3	0.333
#define FINAL_AIR_VOLUME3	0.5
#define ROCKET_MASS3		0.0034
#define NOZZLE_RADIUS3		0.012

/* 로켓 발사 파라메터 4 */
#define ANGLE4			30.0
#define INIT_PRESSURE_ATM4	2.0
#define WATER_VOLUME4		0.333
#define TOTAL_VOLUME4		0.5
#define INIT_AIR_VOLUME4	0.167
#define FINAL_AIR_VOLUME4	0.5
#define ROCKET_MASS4		0.0034
#define NOZZLE_RADIUS4		0.012

float atm_to_psi(float pressure)
{
	return pressure * ATM_TO_PSI;
}

float psi_to_newton_per_m2(float pressure)
{
	return pressure * PSI_TO_N_M2;
}

float calc_water_mass(float water_volume)
{
	return water_volume * WATER_DENSITY / 1000.0;
}

float calc_air_volume(float total_volume, float water_volume)
{
	return total_volume - water_volume;
}

float calc_final_air_pressure(float init_pressure, float init_air_volume, float final_air_volume)
{
	return init_pressure * (init_air_volume / final_air_volume);
}

float calc_average_pressure_variation(float init_pressure, float final_pressure)
{
	return (init_pressure + final_pressure) / 2.0;
}

float calc_nozzle_area(float nozzle_radius)
{
	return M_PI * pow(nozzle_radius, 2.0);
}

float calc_average_exit_velocity(float nozzle_area, float average_pressure_variation)
{
	return nozzle_area * DRAG_COEFFICIENT * sqrt(2 * WATER_DENSITY * average_pressure_variation);
}

float calc_water_exit_velocity(float nozzle_area, float average_exit_velocity)
{
	return average_exit_velocity / (WATER_DENSITY * nozzle_area);
}

float calc_rocket_thrust(float water_exit_velocity, float average_exit_velocity)
{
	return average_exit_velocity * water_exit_velocity;
}

float calc_mass_average(float rocket_mass, float water_mass)
{
	return (rocket_mass + water_mass) / 2.0;
}

float calc_net_force(float rocket_thrust, float mass_average)
{
	return rocket_thrust - mass_average * GRAVITY;
}

float calc_rocket_accel(float net_force, float mass_average)
{
	return net_force / mass_average;
}

float calc_water_remove_time(float water_mass, float average_exit_velocity)
{
	return water_mass / average_exit_velocity;
}

float calc_rocket_velocity(float rocket_accel, float water_remove_time)
{
	return rocket_accel * water_remove_time;
}

float angle_to_radian(float angle)
{
	return angle * (M_PI / 180.0);
}

float calc_rocket_ideal_range(float rocket_velocity, float rad_angle)
{
	return pow(rocket_velocity, 2.0) * sin(2 * rad_angle) / GRAVITY;
}

float calc_rocket_range(float rocket_ideal_range)
{
	return rocket_ideal_range * ROCKET_DRAG;
}

void simulate_rocket(float ang, float init_press, float water_vol, float total_vol,
		     float init_air_vol, float final_air_vol, float rocket_m, float nozzle_r)
{
	float angle = ang;
	float sin_angle;			// for range
	float rocket_range;
	float rocket_ideal_range;
	float init_pressure = init_press, final_pressure;
	float water_volume = water_vol;
	float total_volume = total_vol;
	float init_air_volume = init_air_vol, final_air_volume = final_air_vol;
	float water_mass;
	float average_pressure_variation;
	float average_exit_velocity;
	float water_exit_velocity;
	float rocket_thrust;
	float net_force;
	float rocket_accel;
	float water_remove_time;
	float rocket_velocity;
	float rocket_mass = rocket_m;	// bottle_mass
	float mass_average;
	float nozzle_radius = nozzle_r;
	float nozzle_area;

	printf("초기 조건들\n");
	printf("angle = %.1f 도, P_i = %.5f atm, W_m = %.1f kg, T_m = %.1f kg, V_ai = %.1f L,\n" \
		"V_af = %.1f L, R_m = %.4f kg, r_nozzle = %.4f m\n\n", angle, init_pressure, water_volume,
		total_volume, init_air_volume, final_air_volume, rocket_mass, nozzle_radius);

	printf("시뮬레이션 데이터들\n");

	init_pressure = atm_to_psi(init_pressure);

	printf("atm 2 psi = %.5f psi\n", init_pressure);

	water_mass = calc_water_mass(water_volume);

	printf("water_mass = %.7f kg\n", water_mass);

	nozzle_area = calc_nozzle_area(nozzle_radius);

	printf("nozzle_area = %.8f m^2\n", nozzle_area);

	final_pressure = calc_final_air_pressure(init_pressure, init_air_volume, final_air_volume);

	printf("final_pressure = %.5f psi\n", final_pressure);

	average_pressure_variation = calc_average_pressure_variation(init_pressure, final_pressure);

	printf("average_pressure_variation = %.5f psi\n", average_pressure_variation);

	average_pressure_variation = psi_to_newton_per_m2(average_pressure_variation);

	printf("average_pressure_variation = %.5f N/m^2\n", average_pressure_variation);

	average_exit_velocity = calc_average_exit_velocity(nozzle_area, average_pressure_variation);

	printf("average_exit_velocity = %.5f kg/s\n", average_exit_velocity);

	water_exit_velocity = calc_water_exit_velocity(nozzle_area, average_exit_velocity);

	printf("water_exit_velocity = %.5f m/s\n", water_exit_velocity);

	rocket_thrust = calc_rocket_thrust(water_exit_velocity, average_exit_velocity);

	printf("rocket_thrust = %.5f N\n", rocket_thrust);

	mass_average = calc_mass_average(rocket_mass, water_mass);

	printf("mass_average = %.5f kg\n", mass_average);

	net_force = calc_net_force(rocket_thrust, mass_average);

	printf("net_force = %.5f N\n", net_force);

	rocket_accel = calc_rocket_accel(net_force, mass_average);

	printf("rocket_accel = %.5f m/s^2\n", rocket_accel);

	water_remove_time = calc_water_remove_time(water_mass, average_exit_velocity);

	printf("water_remove_time = %.5f 초\n", water_remove_time);

	rocket_velocity = calc_rocket_velocity(rocket_accel, water_remove_time);

	printf("rocket_velocity = %.5f m/s\n", rocket_velocity);

	angle = angle_to_radian(angle);

	printf("angle radian = %.5f rad\n", angle);

	rocket_ideal_range = calc_rocket_ideal_range(rocket_velocity, angle);

	printf("rocket_ideal_range = %.5f m\n", rocket_ideal_range);

	rocket_range = calc_rocket_range(rocket_ideal_range);

	printf("rocket_range = %.5f m\n\n", rocket_range);
}

int main(void)
{
	simulate_rocket(ANGLE1, INIT_PRESSURE_ATM1, WATER_VOLUME1, TOTAL_VOLUME1,
			INIT_AIR_VOLUME1, FINAL_AIR_VOLUME1, ROCKET_MASS1, NOZZLE_RADIUS1);
	
	simulate_rocket(ANGLE2, INIT_PRESSURE_ATM2, WATER_VOLUME2, TOTAL_VOLUME2,
			INIT_AIR_VOLUME2, FINAL_AIR_VOLUME2, ROCKET_MASS2, NOZZLE_RADIUS2);
	
	simulate_rocket(ANGLE3, INIT_PRESSURE_ATM3, WATER_VOLUME3, TOTAL_VOLUME3,
			INIT_AIR_VOLUME3, FINAL_AIR_VOLUME3, ROCKET_MASS3, NOZZLE_RADIUS3);
	
	simulate_rocket(ANGLE4, INIT_PRESSURE_ATM4, WATER_VOLUME4, TOTAL_VOLUME4,
			INIT_AIR_VOLUME4, FINAL_AIR_VOLUME4, ROCKET_MASS4, NOZZLE_RADIUS4);
	
	return 0;
}
