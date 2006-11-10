/*	$Id: test.c,v 1.3 2006/11/10 17:38:33 christos Exp $	*/

/*-
 * Copyright (c)2004 Citrus Project,
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

#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <err.h>

typedef size_t wcrtomb_t(char *, wchar_t, mbstate_t *);

wcrtomb_t _wctomb;
void dotest1(const char *, wcrtomb_t, wchar_t *, mbstate_t *);

size_t
_wctomb(char *s, wchar_t wc, mbstate_t *ps)
{

	return wctomb(s, wc);
}

int
main(int argc, char *argv[])
{
	wchar_t wcs[teststring_wclen + 2];
	const char *pcs;
	size_t sz;
	mbstate_t st;

	if (setlocale(LC_CTYPE, teststring_loc) == NULL)
		exit(1);

	pcs = teststring;
	wcs[teststring_wclen] = L'X'; /* poison */
	sz = mbsrtowcs(wcs, &pcs, teststring_wclen + 2, NULL);
	if (sz != teststring_wclen)
		exit(3);
	if (wcs[teststring_wclen])
		exit(4);

	dotest1("wctomb", _wctomb, wcs, NULL);
	memset(&st, 0, sizeof(st));
	dotest1("wcrtomb", wcrtomb, wcs, &st);
	dotest1("wcrtomb (internal state)", wcrtomb, wcs, NULL);

	exit(0);
}

void
dotest1(const char *text, wcrtomb_t fn, wchar_t *wcs, mbstate_t *stp)
{
	char *cs;
	int ret;
	int i;

	cs = malloc(MB_CUR_MAX);
	if (cs == NULL)
		err(1, NULL);
	printf("testing %s\n", text);

	for (i = 0; i < teststring_wclen + 1; i++) {
		ret = fn(cs, wcs[i], stp);
		if (ret != teststring_mblen[i]) {
			printf("\t[%d] %d != %d BAD\n",
			    i, ret, teststring_mblen[i]);
			exit(5);
		}
#if 0
		printf("\t[%d] %d ok\n", i, ret);
#endif
	}
	free(cs);
}
