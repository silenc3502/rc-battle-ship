#include <time.h>
#include <stdio.h>
#include <stdlib.h>

float get_prob(float p, float t)
{
	return p / t;
}

void init_val(float *p, float *t)
{
	*t = rand() % 100 + 1;
redo:
	*p = rand() % 100 + 1;
	if(*p > *t)
		goto redo;
}

int main(void)
{
	float p = 0, q = 0;
	float part, total;

	srand(time(NULL));

	init_val(&part, &total);

	p = get_prob(part, total);
	printf("p = %.2f\n", p * 100.0);
	printf("q = %.2f\n", 100.0 - p * 100.0);

	return 0;
}
