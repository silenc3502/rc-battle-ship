#include <math.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct prob_dist_table
{
	char name[31];
	float prob;
	int money;
} pdt;

char *name_table[5] = {"tripple_dollar", "double_dollar_cherry", "tripple_cherry", "tripple_lemon", "fail"};

char *twenty[3] = {"dollar", "dollar", "dollar"};
char *fifteen[3][3] = {{"dollar", "dollar", "cherry"},
		       {"dollar", "cherry", "dollar"},
		       {"cherry", "dollar", "dollar"}};
char *ten[3] = {"cherry", "cherry", "cherry"};
char *five[3] = {"lemon", "lemon", "lemon"};

int check_res(char **arr)
{
	int i, j;
	int ds = 0, cds = 0, cs = 0, ls = 0;

#if 0
	char *test[3] = {"dollar", "dollar", "dollar"};

	char *twenty[3] = {"dollar", "dollar", "dollar"};
	char *fifteen[3][3] = {{"dollar", "dollar", "cherry"},
			       {"dollar", "cherry", "dollar"},
			       {"cherry", "dollar", "dollar"}};
	char *ten[3] = {"cherry", "cherry", "cherry"};
	char *five[3] = {"lemon", "lemon", "lemon"};
#endif

#if 1
	for(i = 0; i < 3; i++)
		printf("arr[%d] = %s\t", i, arr[i]);
	printf("\n");
#endif

	for(i = 0; i < 3; i++)
	{
		if(!(memcmp(twenty[i], arr[i], strlen(twenty[i]))))
			ds++;
		else if(!(memcmp(ten[i], arr[i], strlen(ten[i]))))
			cs++;
		else if(!(memcmp(five[i], arr[i], strlen(five[i]))))
			ls++;
	}

	printf("ds = %d\t, cs = %d\t, ls = %d\n", ds, cs, ls);

	if(ds == 3)
		return 19;
	else if(ds == 2 && cs == 1)
		return 14;
	else if(cs == 3)
		return 9;
	else if(ls == 3)
		return 4;

#if 0
	if(!(memcmp(twenty, test, 18)))
		printf("test: ddd\n");
#endif

	return -1;
}

float get_prob(char **arr)
{
	int i;
	float tmp = 1;

	for(i = 0; i < 3; i++)
	{
		if(!(strcmp(arr[i], "dollar")))
			tmp *= 0.1;
		if(!(strcmp(arr[i], "cherry")))
			tmp *= 0.2;
		if(!(strcmp(arr[i], "lemon")))
			tmp *= 0.2;
		if(!(strcmp(arr[i], "etc")))
			tmp *= 0.5;
	}

	//printf("tmp = %f\n", tmp);

	return tmp;
}

void slot_machine(char **d, int *money)
{
	float prob;
	int i, idx, len;
	char *arr[3] = {0};

	for(i = 0; i < 3; i++)
	{
		idx = rand() % 4;
		len = strlen(d[idx]);
		arr[i] = (char *)malloc(sizeof(char *) * (len));
		strcpy(arr[i], d[idx]);
	}

#if 0
	for(i = 0; i < 3; i++)
		printf("arr[%d] = %s\n", i, arr[i]);
#endif

	prob = get_prob(arr);
	printf("prob = %.2f%c\n", prob * 100.0, 0x25);

	*money = check_res(arr);
}

void set_prob_table(pdt *t)
{
	int i, j;
#if 0
	int flag = 0;
#endif
	float tmp;
	//char *(*)[3] tmp_arr[4] = 
	//(char *(*)[3])

	char ** tmp_arr[4] = {twenty, fifteen[0], ten, five};
#if 0
	char (** tmp_arr[4])[4] = {twenty, fifteen[0], ten, five};
#endif
	//char (** tmp_arr[3])[3] = {twenty, ten, five};
	//char **tmp_arr[4] = {twenty, fifteen, ten, five};

	for(i = 0; i < 5; i++)
	{
		strcpy(t[i].name, name_table[i]);
		printf("t[%d].name = %s\n", i, t[i].name);

		if(i == 4)
		{
			float tmp = 0;

			for(j = 0; j < i; j++)
				tmp += t[j].prob;

			t[i].prob = 1 - tmp;

			goto print;
		}

#if 0
		if(i == 1 || i == 4)
			continue;

		if(i == 3)
		{
			i = 2;
			flag = 1;
		}
#endif

		tmp = 1;
		for(j = 0; j < 3; j++)
		{
			if(!strcmp(tmp_arr[i][j], "dollar"))
				tmp *= 0.1;
			else if(!strcmp(tmp_arr[i][j], "cherry"))
				tmp *= 0.2;
			else if(!strcmp(tmp_arr[i][j], "lemon"))
				tmp *= 0.2;
		}

#if 0
		if(flag)
		{
			i = 3;
			flag = 0;
		}
#endif

		if(i == 1)
			tmp *= 3;

		t[i].prob = tmp;

print:
		printf("t[%d].prob = %.3f\n", i, t[i].prob);
	}

	//printf("tmp_arr[0][0] = %s\n", tmp_arr[0][0]);
}

void set_money_table(pdt *t)
{
	int i = 0;

	t[i++].money = check_res(twenty);
	t[i++].money = check_res(fifteen[0]);
	t[i++].money = check_res(ten);
	t[i++].money = check_res(five);
	t[i].money = -1;

	for(i = 0; i < 5; i++)
		printf("t[%d].money = %d\n", i, t[i].money);
}

float calc_expectation(pdt *t)
{
	int i;
	float tmp = 0;

	for(i = 0; i < 5; i++)
		tmp += t[i].prob * t[i].money;

	return tmp;
}

float calc_var(pdt *t, float m)
{
	int i;
	float tmp = 0;

	for(i = 0; i < 5; i++)
		tmp += pow((t[i].money - m), 2.0) * t[i].prob;

	return tmp;
}

float calc_std_dev(float var)
{
	return sqrt(var);
}

float linear_expect(int mult, pdt *t, float expect, int new_pay)
{
	return mult * (expect - t[4].money) - new_pay;
}

float linear_var(int mult, float var)
{
	return pow((float)mult, 2.0) * var;
}

float linear_std_dev(int mult, float std_dev)
{
	return mult * std_dev;
}

int main(void)
{
	char *data[4] = {"dollar", "cherry", "lemon", "etc"};
	int i, money = 0, new_pay = 2;
	float expect, var, std_dev;
	float new_expect, new_var, new_std_dev;
	pdt table[5];

	srand(time(NULL));	

	for(i = 0; i < 10; i++)
	{
		slot_machine(data, &money);
		printf("money = %d\n", money);
		//printf("expect = %f\n", expect);
	}

	set_prob_table(table);
	set_money_table(table);
	expect = calc_expectation(table);
	printf("expectation = %.3f\n", expect);

	var = calc_var(table, expect);
	printf("variance = %.5f\n", var);

	std_dev = calc_std_dev(var);
	printf("Standard Deviation = %.7f\n", std_dev);

	new_expect = linear_expect(5, table, expect, new_pay);
	printf("new expectation = %.3f\n", new_expect);

	new_var = linear_var(5, var);
	printf("new variance = %.5f\n", new_var);

	new_std_dev = linear_std_dev(5, std_dev);
	printf("New Standard Deviation = %.7f\n", new_std_dev);

	return 0;
}
