#include <stdio.h>
#include <stdlib.h>

int main(void)
{
	int i;
	char *pab = NULL;
	char *pab_buf = (char *)malloc(MB_CUR_MAX);
	wchar_t *puc_buf = L"Hello";
	wchar_t *puc = (wchar_t *)malloc(sizeof(wchar_t));

	printf("Convert to Multibyte String\n");
	i = wcstombs(pab_buf, puc_buf, MB_CUR_MAX);
	printf("Characters Converted: %d\n", i);
	printf("Hex Value of First");
	printf("Multi-Byte Character: %#.4x\n\n", pab_buf[0]);

	printf("Convert back to Wide-Character String:\n");
	i = mbstowcs(puc, pab_buf, MB_CUR_MAX);
	printf("Characters Converted: %d\n", i);
	printf("Hex Value of First");
	printf("Wide Character: %#.4x\n\n", puc[0]);

	printf("sizeof(wchar_t) = %lu\n", sizeof(wchar_t));
	printf("sizeof(char) = %lu\n", sizeof(char));

	return 0;
}
