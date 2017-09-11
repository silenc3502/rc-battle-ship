#include <time.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct prob
{
	int count;
	float p;
	float q;
} pg;

float total;

void init_pg(pg *e1, pg *e2)
{
	float tmp1, tmp2;
redo:
	tmp1 = rand() % 100 + 1;
	tmp2 = rand() % 100 + 1;
	if(tmp1 + tmp2 > total)
		goto redo;

	e1->count = tmp1;
	e1->p = tmp1 / total;
	e1->q = 1 - e1->p;

	e2->count = tmp2;
	e2->p = tmp2 / total;
	e2->q = 1 - e2->p;
}

void print_pg(pg g)
{
	printf("count = %d\n", g.count);
	printf("p = %.2f\n", g.p * 100.0);
	printf("q = %.2f\n", 100.0 - g.p * 100.0);
}

pg mutual_exclusive_event_add(pg e1, pg e2)
{
	pg res;

	res.count = e1.count + e2.count;
	res.p = e1.p + e2.p;
	res.q = 1.0 - res.p;

	return res;
}

float init_total(void)
{
	return rand() % 100 + 1;
}

int init_rand_intersect(pg e1, pg e2)
{
	int tmp;

	printf("e1.count = %d\n", e1.count);
	printf("e2.count = %d\n", e2.count);
redo:
	tmp = rand() % 100 + 1;

	if(e1.count > e2.count)
	{
		if(tmp > e2.count)
		{
			goto redo;
		}
	}
	else if(e1.count < e2.count)
	{
		if(tmp > e1.count)
		{
			goto redo;
		}
	}
	else
	{
		if(tmp > e1.count)
		{
			goto redo;
		}
	}

	return tmp;
}

pg intersect_event_add(pg e1, pg e2, int intersect)
{
	pg res;

	res.count = e1.count + e2.count - intersect;
	res.p = res.count / total;
	res.q = 1 - res.p;

	return res;
}

int main(void)
{
	pg res, red, black;
	int intersect;

	srand(time(NULL));

	total = init_total();
	printf("total = %f\n", total);
	init_pg(&red, &black);

	print_pg(red);
	putchar('\n');
	print_pg(black);
	putchar('\n');

	res = mutual_exclusive_event_add(red, black);
	print_pg(res);
	putchar('\n');

	intersect = init_rand_intersect(red, black);
	printf("intersect = %d\n", intersect);

	res = intersect_event_add(red, black, intersect);
	print_pg(res);

	return 0;
}
