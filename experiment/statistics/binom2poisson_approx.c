#include <math.h>
#include <stdio.h>

double e = 2.71828182845904523536028747135266249775724709369995957496696762772407663035354759;

typedef enum {false, true} bool;

typedef struct poisson_dist
{
        float prob;
        float lambda;
        int event_num;

        float expect;
        float var;
} pd;

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

	bool approx;
} bd;

long int factorial(int n)
{
	long int tmp = 1;

	while(n)
		tmp *= n--;

	return tmp;
}

void init_poisson_dist(pd *poisson, float lambda, int r)
{
        poisson->lambda = lambda;
        poisson->event_num = r;
}

void calc_poisson_prob(pd *poisson)
{
        poisson->prob = pow(e, -(poisson->lambda)) *
                        pow(poisson->lambda, poisson->event_num) /
                        factorial(poisson->event_num);

	printf("poisson->lambda = %f\n", poisson->lambda);
	printf("pow(e, -(poisson->lambda)) = %f\n", pow(e, -(poisson->lambda)));
}

void init_binom_dist(bd *binom, float p, int n, int r)
{
	binom->p = p;
	binom->q = 1 - p;
	binom->n = n;
	binom->r = r;

	if(n >= 50 && p <= 0.1)
		binom->approx = true;
	else
		binom->approx = false;
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

void calc_binom_prob(bd *binom)
{
	float res;

	if(binom->approx)
	{
		pd poisson = {0};
		// 70 개 중 5 개를 찾아낼 확률
		init_poisson_dist(&poisson, binom->expect, 5);
		calc_poisson_prob(&poisson);
		//printf("poisson.prob = %f\n", poisson.prob);
		binom->prob = poisson.prob;
		printf("true\n");
	}
	else
	{
		res = opt_combination(binom->n, binom->r);
		binom->prob = res * pow(binom->p, binom->r) * pow(binom->q, binom->n - binom->r);
		printf("false\n");
	}
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

	init_binom_dist(&binom, 0.05, 70, 5);

	calc_expect(&binom);
	calc_var(&binom);
	calc_binom_prob(&binom);
	print_binom_dist(binom);

	return 0;
}
