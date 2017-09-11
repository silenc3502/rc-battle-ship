#include <stdio.h>

void size_check(void)
{
	printf("sizeof(int) = %lu\n", sizeof(int));
	printf("sizeof(long) = %lu\n", sizeof(long));
	printf("sizeof(long int) = %lu\n", sizeof(long int));
	printf("sizeof(long double) = %lu\n", sizeof(long double));
}

long int factorial(int n)
{
	long int tmp = 1;

	while(n)
		tmp *= n--;

	return tmp;
}

long int permutation(int n, int r)
{
	long int numerator = factorial(n);
	long int denominator = factorial(n - r);
	printf("numerator = %ld\n", numerator);
	printf("denominator = %ld\n", denominator);

	return numerator / denominator;
}

long int opt_permutation(int n, int r)
{
	int i;
	int start = n;
	int end = n - r;
	int tmp = 1;

	for(i = start; i > end; i--)
		tmp *= i;

	return tmp;
}

long int combination(int n, int r)
{
	long int numerator = factorial(n);
	long int denominator = factorial(n - r) * factorial(r);

	return numerator / denominator;
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

int main(void)
{
	long int res;

	size_check();
	res = permutation(20, 3);
	printf("20_P_3 = %ld\n", res);

	res = opt_permutation(20, 3);
	printf("20_P_3 = %ld\n", res);

	res = combination(12, 5);
	printf("12_C_5 = %ld\n", res);

	res = opt_combination(12, 5);
	printf("12_C_5 = %ld\n", res);

	return 0;
}
