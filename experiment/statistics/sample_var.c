#include <math.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#define LEN		1000
#define SAM_LEN		20

float arr[LEN];
float sample[SAM_LEN];

void init_arr(void)
{
	int i;

	for(i = 0; i < LEN; i++)
		arr[i] = rand() % 100 + 1;
}

void init_sample(void)
{
	int i;

	for(i = 0; i < SAM_LEN; i++)
		sample[i] = arr[rand() % LEN];
}

void print_sample(void)
{
	int i;

	for(i = 0; i < LEN; i++)
		printf("sample[%d] = %f\n", i, sample[i]);
}

float get_arr_mean(void)
{
	int i;
	float sum = 0;

	for(i = 0; i < LEN; i++)
		sum += arr[i];

	return sum / LEN;
}

float get_sam_mean(void)
{
	int i;
	float sum = 0;

	for(i = 0; i < SAM_LEN; i++)
		sum += sample[i];

	return sum / SAM_LEN;
}

float get_arr_var(float am)
{
	int i;
	float sum = 0;

	for(i = 0; i < LEN; i++)
		sum += pow((arr[i] - am), 2.0);

	return sum / LEN;
}

float get_sam_var(float sm)
{
	int i;
	float sum = 0;

	for(i = 0; i < SAM_LEN; i++)
		sum += pow((sample[i] - sm), 2.0);

	return sum / (SAM_LEN - 1);
}

float get_arr_std_dev(float avar)
{
	return sqrt(avar);
}

float get_sam_std_dev(float svar)
{
	return sqrt(svar);
}

int main(void)
{
	float am, sm, avar, svar, astd_dev, sstd_dev;
	srand(time(NULL));
	init_arr();
	init_sample();
	print_sample();

	am = get_arr_mean();
	printf("real mean = %f\n", am);
	sm = get_sam_mean();
	printf("sample mean = %f\n", sm);

	avar = get_arr_var(am);
	printf("real var = %f\n", avar);
	svar = get_sam_var(sm);
	printf("sample var = %f\n", svar);

	astd_dev = get_arr_std_dev(avar);
	printf("real standard deviation = %f\n", astd_dev);
	sstd_dev = get_sam_std_dev(svar);
	printf("sample standard deviation = %f\n", sstd_dev);

	return 0;
}
