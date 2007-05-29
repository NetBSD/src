/*      $NetBSD: multibyte.c,v 1.2 2007/05/29 17:46:44 he Exp $      */

/*
 * Ignore all multibyte sequences, removes all the citrus code.
 * Probably only used by vfprintf() when parsing the format string.
 */

#include <wchar.h>

size_t
mbrtowc(wchar_t *wc, const char *str, size_t max_sz, mbstate_t *ps)
{
	return (*wc = *str) == 0 ? 0 : 1;
}

size_t
wcrtomb(char *str, wchar_t wc, mbstate_t *ps)
{
    *str = wc;
    return 1;
}

int
wctob(wint_t x)
{
	return x;
}

wint_t
btowc(int x) {
	return x;
}

size_t
mbrlen(const char * __restrict p, size_t l, mbstate_t * __restrict v)
{
	size_t i;
	for (i = 0; i < l; i++)
		if (p[i] == '\0')
			return i;
	return l;
}

int
mbsinit(const mbstate_t *s)
{
	return 0;
}

size_t
mbsrtowcs(wchar_t * __restrict pwcs, const char ** __restrict s, size_t n,
    mbstate_t * __restrict ps)
{
	const char *p;
	wchar_t *d;
	size_t count;

	for (p = *s, d = pwcs, count = 0; 
		count <= n;
		count++, d++, p++)
	{
		if (mbrtowc(d, p, 1, ps) == 0)
			break;
	}
	return count;
}

size_t
wcsrtombs(char * __restrict s, const wchar_t ** __restrict pwcs, size_t n,
    mbstate_t * __restrict ps)
{
	char *d;
	const wchar_t *p;
	size_t count;

	for (p = *pwcs, d = s, count = 0;
		count <= n && *p != 0;
		count++, d++, p++)
	{
		wcrtomb(d, *p, ps);
	}
	*d = 0;
	return count;
}
