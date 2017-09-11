#include <stdlib.h>
#include <stdio.h>

#define BUFFER_SIZE	100

int main(void)
{
	size_t count;
	char *p_buf = (char *)malloc(BUFFER_SIZE);
	wchar_t *p_ubuf = L"UNICODE 2 ASCII";

	printf("Convert Wide Character String\n");

	count = wcstombs(p_buf, p_ubuf, BUFFER_SIZE);

	printf("Characters Converted: %lu\n", count);
	printf("Multibyte Character: %s\n", p_buf);
	free(p_buf);

	return 0;
}
