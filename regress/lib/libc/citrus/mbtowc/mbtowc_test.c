/* From: Miloslav Trmac <mitr@volny.cz> */
#include <langinfo.h>
#include <limits.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>

int
main(void)
{
	wchar_t wc;
	char back[MB_LEN_MAX];
	int ret;
	size_t i;

	setlocale(LC_ALL, "");
	printf("Charset: %s\n", nl_langinfo(CODESET));
	ret = mbtowc(&wc, "\xe4", 1);
	printf("mbtowc(): %d\n", ret);
	if(ret > 0) {
		printf("Result: 0x%08lX\n",(unsigned long)wc);
		ret = wctomb(back, wc);
		printf("wctomb(): %d\n", ret);
		for(i = 0; ret > 0 && i <(size_t)ret; i++)
			printf("%02X ",(unsigned char)back[i]);
		putchar('\n');
		return EXIT_SUCCESS;
	} else
		return EXIT_FAILURE;
}
