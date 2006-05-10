/*	$NetBSD: ctype1.c,v 1.5 2006/05/10 19:07:22 mrg Exp $	*/

/*-
 * Copyright (c)2003 Citrus Project,
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <locale.h>
#include <wchar.h>
#include <string.h>

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
