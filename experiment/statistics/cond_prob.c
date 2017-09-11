#include <stdio.h>

float total = 38;
float tot_even;
float tot_odd;

#define	RED		1
#define BLACK		2
#define GREEN		3

#define ODD		0
#define EVEN		1

int data[38] = {1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 2, 1,
		2, 1, 2, 1, 2, 1, 1, 2, 1, 2, 1, 2,
		1, 2, 1, 2, 2, 1, 2, 1, 2, 1, 2, 1, 3, 3};

int count_data(int color)
{
	int i, cnt = 0;

	for(i = 0; i < total; i++)
		if(data[i] == color)
			cnt++;

	return cnt;
}

void count_eo(int *e, int *o)
{
	int i, cnt = 0;

	for(i = 0; i < total - 2; i++)
	{
		if(i % 2)
		{
			(*e)++;
		}
		else
		{
			(*o)++;
		}
	}
}

void count_cond_data(int color, int *e, int *o)
{
	int i, cnt = 0;

	for(i = 0; i < total - 2; i++)
	{
		if((i % 2) && (data[i] == color))
		{
			(*e)++;
		}
		else if(!(i % 2) && (data[i] == color))
		{
			(*o)++;
		}
	}
}

void print_prob(int data)
{
	printf("prob = %.2f%c\n", (float)data * 100.0 / total, 0x25);
}

float bayes_prob(int target, int src, float denom, int flag)
{
	float tmp;

	if(flag)
		tmp = (((float)target / total) * ((float)src / tot_even)) / denom;
	else
		tmp = (((float)target / total) * ((float)src / tot_odd)) / denom;
		
	return tmp;
}

float get_eo_prob(int p1, int p2, int p3, int p4, int flag)
{
	float tmp;

	if(flag)
		tmp = ((float)p1 / total) * ((float)p2 / tot_even) + ((float)p3 / total) * ((float)p4 / tot_even);
	else
		tmp = ((float)p1 / total) * ((float)p2 / tot_odd) + ((float)p3 / total) * ((float)p4 / tot_odd);

	return tmp;
}

int main(void)
{
	int red, black, green, even = 0, odd = 0;
	int black_even = 0, black_odd = 0, red_even = 0, red_odd = 0;
	float decision_be_how_black, decision_bo_how_black;
	float decision_re_how_red, decision_ro_how_red;
	float even_prob, odd_prob;

	red = count_data(RED);
	black = count_data(BLACK);
	green = count_data(GREEN);

	printf("red = %d\n", red);
	print_prob(red);
	printf("black = %d\n", black);
	print_prob(black);
	printf("green = %d\n", green);
	print_prob(green);

	count_eo(&even, &odd);
	printf("even = %d\n", even);
	print_prob(even);
	printf("odd = %d\n", odd);
	print_prob(odd);

	tot_even = even;
	tot_odd = odd;

	count_cond_data(BLACK, &black_even, &black_odd);
	count_cond_data(RED, &red_even, &red_odd);
	printf("black_even = %d\n", black_even);
	print_prob(black_even);
	printf("black_odd = %d\n", black_odd);
	print_prob(black_odd);
	printf("red_even = %d\n", red_even);
	print_prob(red_even);
	printf("red_odd = %d\n", red_odd);
	print_prob(red_odd);

	// 짝수 및 홀수 구하기
	even_prob = get_eo_prob(black, black_even, red, red_even, EVEN);
	printf("even_prob = %.2f%c\n", even_prob * 100.0, 0x25);
	odd_prob = get_eo_prob(black, black_odd, red, red_odd, ODD);
	printf("odd_prob = %.2f%c\n", odd_prob * 100.0, 0x25);

	// 검정색 짝수가 A 만큼 발생했을 때 검정색이 얼만큼 발생할지에 대한 확률
	// P(A | B) = [P(A) * P(B | A)] / [P(A) * P(B | A) + P(A') * P(B | A')]
	decision_be_how_black = bayes_prob(black, black_even, even_prob, EVEN);
	printf("P(black | black_even) = %.2f%c\n", decision_be_how_black * 100.0, 0x25);
	decision_bo_how_black = bayes_prob(black, black_odd, odd_prob, ODD);
	printf("P(black | black_odd) = %.2f%c\n", decision_bo_how_black * 100.0, 0x25);
	decision_re_how_red = bayes_prob(red, red_even, even_prob, EVEN);
	printf("P(red | red_even) = %.2f%c\n", decision_re_how_red * 100.0, 0x25);
	decision_ro_how_red = bayes_prob(red, red_odd, odd_prob, ODD);
	printf("P(red | red_odd) = %.2f%c\n", decision_ro_how_red * 100.0, 0x25);

	return 0;
}
