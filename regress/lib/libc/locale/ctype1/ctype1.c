/*	$NetBSD: ctype1.c,v 1.3 2004/02/27 15:14:58 itojun Exp $	*/

#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <locale.h>
#include <wchar.h>

int
main(int ac, char **av)
{
	char	buf[256];
	char	*str;
	int	c;
	wchar_t	wbuf[256];
	wchar_t	*p;

	str = setlocale(LC_ALL, "");
	if (str == 0)
		err(1, "setlocale");
	fprintf(stderr, "===> Testing for locale %s... ", str);

	for (str = buf; 1; str++) {
		c = getchar();
		if (c == EOF || c == '\n')
			break;
		*str = c;
	}
	*str = '\0';
	strcat(buf, "\n");

	mbstowcs(wbuf, buf, sizeof(buf) / sizeof(buf[0]) - 1);
	wcstombs(buf, wbuf, sizeof(wbuf) / sizeof(wbuf[0]) - 1);
	printf("%s\n", buf);

	/*
	 * The output here is implementation-dependent.
	 * When we replace the conversion routine, we might have to
	 * update the *.exp files.
	 */
	for (p = wbuf; *p; p++) {
		printf("0x%04X  ", (unsigned)*p);
	}
	putchar('\n');

	printf("width:\n", buf);
	for (p = wbuf; *p; p++) {
		printf("%d ", wcwidth(*p));
	}
	putchar('\n');

	printf("wcswidth=%d\n",
	    wcswidth(wbuf, sizeof(wbuf) / sizeof(wbuf[0]) - 1));

	return 0;
}
