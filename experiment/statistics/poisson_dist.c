#include <math.h>
#include <stdio.h>

double e = 2.71828182845904523536028747135266249775724709369995957496696762772407663035354759;

typedef struct poisson_dist
{
	float prob;
	float lambda;
	int event_num;

	float expect;
	float var;
} pd;

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

void calc_prob(pd *poisson)
{
	poisson->prob = pow(e, -(poisson->lambda)) *
			pow(poisson->lambda, poisson->event_num) /
			factorial(poisson->event_num);
}

void set_event_num(pd *poisson, int num)
{
	poisson->event_num = num;
}

void calc_expect(pd *poisson)
{
	poisson->expect = poisson->lambda;
}

void calc_var(pd *poisson)
{
	poisson->var = poisson->lambda;
}

void print_poisson_dist(pd poisson)
{
	printf("poisson prob = %.5f%c\n", poisson.prob * 100.0, 0x25);
	printf("poisson lambda = %.3f\n", poisson.lambda);
	printf("poisson event_num = %d\n", poisson.event_num);
	printf("poisson expect = %.3f\n", poisson.expect);
	printf("poisson var = %.3f\n", poisson.var);
}

int main(void)
{
	pd poisson = {0};

	init_poisson_dist(&poisson, 2, 3);

	calc_prob(&poisson);
	calc_expect(&poisson);
	calc_var(&poisson);
	print_poisson_dist(poisson);

	printf("*************** New ***************\n");
	set_event_num(&poisson, 5);

	calc_prob(&poisson);
	calc_expect(&poisson);
	calc_var(&poisson);
	print_poisson_dist(poisson);

	return 0;
}
