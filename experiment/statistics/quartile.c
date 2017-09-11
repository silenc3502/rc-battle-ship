#include <stdio.h>
#include <stdlib.h>

#define LEN		20
#define LOWER		LEN / 4
#define UPPER		3 * LEN / 4

typedef struct quartile
{
	float lower;
	float lower_idx;
	float median;
	float upper;
	float upper_idx;
} quartile;

float get_median(float *arr)
{
	if(LEN % 2)
		return arr[LEN / 2];
	else
		return (arr[LEN / 2] + arr[LEN / 2 - 1]) / 2.0;
}

quartile *get_quartile(float *arr)
{
	quartile *q = (quartile *)malloc(sizeof(quartile));;

	if(LEN % 4)
	{
		q->lower_idx = LOWER - 1;
		q->lower = arr[LOWER - 1];
	}
	else
	{
		q->lower_idx = LOWER + 0.5;
		q->lower = (arr[LOWER] + arr[LOWER - 1]) / 2.0;
	}

	q->median = get_median(arr);

	if((3 * LEN) % 4)
	{
		q->upper_idx = UPPER - 1;
		q->upper = arr[UPPER - 1];
	}
	else
	{
		q->upper_idx = UPPER + 0.5;
		q->upper = (arr[UPPER] + arr[UPPER - 1]) / 2.0;
	}

	return q;
}

void print_quartile(quartile *q)
{
	printf("lower quartile = %f\n", q->lower);
	printf("median = %f\n", q->median);
	printf("upper quartile = %f\n", q->upper);
}

int get_quartile_range(quartile *q)
{
	return q->upper_idx - q->lower_idx;
}

int main(void)
{
	int range;
	float arr[LEN] = {1, 1, 1, 2, 2, 3, 3, 3, 3, 3,
			  4, 4, 4, 4, 4, 5, 5, 5, 5, 11};

	quartile *q = NULL;
	q = get_quartile(arr);
	print_quartile(q);

	range = get_quartile_range(q);
	printf("quartile range = %d\n", range);

	return 0;
}
