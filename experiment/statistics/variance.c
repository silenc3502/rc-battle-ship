#include <time.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define LEN		10

float arr[10];

void init_arr(void)
{
	int i;

	for(i = 0; i < LEN; i++)
		arr[i] = rand() % 100 + 1;
}

void print_arr(void)
{
	int i;

	for(i = 0; i < LEN; i++)
		printf("arr[%d] = %f\n", i, arr[i]);

	printf("\n");
}

float get_mean(void)
{
	int i;
	float sum = 0;

	for(i = 0; i < LEN; i++)
		sum += arr[i];

	return sum / LEN;
}

float get_variance(float m)
{
	int i;
	float sum = 0;

	for(i = 0; i < LEN; i++)
		sum += pow((arr[i] - m), 2.0);

	return sum / LEN;
}

float get_std_dev(float var)
{
	return sqrt(var);
}

int main(void)
{
	float mean, var, std_dev;
	srand(time(NULL));

	init_arr();
	print_arr();

	mean = get_mean();
	printf("mean = %f\n", mean);

	var = get_variance(mean);
	printf("var = %f\n", var);

	std_dev = get_std_dev(var);
	printf("std_dev = %f\n", std_dev);

	return 0;
}
