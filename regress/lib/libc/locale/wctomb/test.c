/*	$Id: test.c,v 1.1 2004/09/26 03:45:10 yamt Exp $	*/

#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

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
	char cs[MB_CUR_MAX];
	int ret;
	int i;

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
}
