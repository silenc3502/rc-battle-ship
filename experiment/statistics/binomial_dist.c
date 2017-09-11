#include <math.h>
#include <stdio.h>

typedef struct combination
{
	int n;
	int r;
	float comb;
} comb;

typedef struct binom_dist
{
	float p;
	float q;

	int r;
	int n;

	float prob;
	float expect;
	float var;
} bd;

void init_binom_dist(bd *binom, float p, int n, int r)
{
	binom->p = p;
	binom->q = 1 - p;
	binom->n = n;
	binom->r = r;
}

void init_comb(comb *c, float n, float r)
{
	c->n = n;
	c->r = r;
}

void calc_expect(bd *binom)
{
	binom->expect = binom->n * binom->p;
}

void calc_var(bd *binom)
{
	binom->var = binom->n * binom->p * binom->q;
}

long int factorial(int n)
{
	long int tmp = 1;

	while(n)
		tmp *= n--;

	return tmp;
}

long int opt_combination(int n, int r)
{
	int i;
	int start = n;
	int end = n - r;
	int tmp = 1;

	for(i = start; i > end; i--)
		tmp *= i;

	return tmp / factorial(r);
}

void calc_prob(bd *binom)
{
	float res;

	//init_comb(&c, binom->n, binom->r);
	res = opt_combination(binom->n, binom->r);

	binom->prob = res * pow(binom->p, binom->r) * pow(binom->q, binom->n - binom->r);
}

void print_binom_dist(bd binom)
{
	printf("p = %.2f\n", binom.p);
	printf("q = %.2f\n", binom.q);
	printf("n = %d\n", binom.n);
	printf("r = %d\n", binom.r);
	printf("expect = %.2f\n", binom.expect);
	printf("var = %.4f\n", binom.var);
	printf("prob = %.3f%c\n", binom.prob * 100.0, 0x25);
}

int main(void)
{
	bd binom = {0};

	init_binom_dist(&binom, 0.25, 5, 3);

	calc_expect(&binom);
	calc_var(&binom);
	calc_prob(&binom);
	print_binom_dist(binom);

	return 0;
}
