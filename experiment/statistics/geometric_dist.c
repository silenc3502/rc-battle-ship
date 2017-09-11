#include <math.h>
#include <stdio.h>

typedef struct geo_dist
{
	float p;
	float q;
	int r;

	float expectation;
	float variance;

	float prob;
} gd;

void init_geo_dist(gd *geo, float p, int r)
{
	geo->p = p;
	geo->q = 1 - p;
	geo->r = r;
}

void print_geo_dist_basic_info(gd geo)
{
	printf("geo p = %.2f\n", geo.p);
	printf("geo q = %.2f\n", geo.q);
	printf("geo r = %d\n", geo.r);
}

void calc_expect(gd *geo)
{
	geo->expectation = 1 / geo->p;
}

void calc_variance(gd *geo)
{
	geo->variance = geo->q / pow(geo->p, 2.0);
}

void calc_prob(gd *geo)
{
	geo->prob = geo->p * pow(geo->q, geo->r - 1.0);
}

void print_geo_dist_info(gd geo)
{
	printf("geo p = %.2f\n", geo.p);
	printf("geo q = %.2f\n", geo.q);
	printf("geo r = %d\n", geo.r);

	printf("geo expectation = %f\n", geo.expectation);
	printf("geo variance = %f\n", geo.variance);
	printf("geo prob = %f%c\n", geo.prob * 100.0, 0x25);
}

int main(void)
{
	gd geo = {0};

	init_geo_dist(&geo, 0.2, 3);
	//print_geo_dist_basic_info(geo);

	calc_expect(&geo);
	calc_variance(&geo);
	calc_prob(&geo);
	print_geo_dist_info(geo);

	return 0;
}
