/*      $NetBSD: multibyte.c,v 1.9 2019/08/01 12:28:53 martin Exp $      */

/*
 * Ignore all multibyte sequences, removes all the citrus code.
 * Probably only used by vfprintf() when parsing the format string.
 * And possibly from libcurses if compiled with HAVE_WCHAR.
 */

#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>
#include <ctype.h>

size_t
mbrtowc(wchar_t *wc, const char *str, size_t max_sz, mbstate_t *ps)
{
	if (str == NULL)
		return 0;

	if (wc != NULL)
		*wc = (unsigned char)*str;

	return *str == '\0' ? 0 : 1;
}

size_t
mbrtowc_l(wchar_t *wc, const char *str, size_t max_sz, mbstate_t *ps, locale_t loc)
{
	return mbrtowc(wc, str, max_sz, ps);
}

size_t
wcrtomb(char *str, wchar_t wc, mbstate_t *ps)
{
    *str = wc & 0xFF;
    return 1;
}


size_t
wcrtomb_l(char *str, wchar_t wc, mbstate_t *ps, locale_t loc)
{
	return wcrtomb(str, wc, ps);
}

int
wctob(wint_t x)
{
	return x;
}

int
wctob_l(wint_t x, locale_t loc)
{
	return x;
}

wint_t
btowc(int x)
{
	return x;
}

wint_t
btowc_l(int x, locale_t loc)
{
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


size_t
mbrlen_l(const char * __restrict p, size_t l, mbstate_t * __restrict v,
    locale_t loc)
{
	return mbrlen(p, l, v);
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
mbsrtowcs_l(wchar_t * __restrict pwcs, const char ** __restrict s, size_t n,
    mbstate_t * __restrict ps, locale_t loc)
{
	return mbsrtowcs(pwcs, s, n, ps);
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

size_t
wcsrtombs_l(char * __restrict s, const wchar_t ** __restrict pwcs, size_t n,
    mbstate_t * __restrict ps, locale_t loc)
{
	return wcsrtombs(s, pwcs, n, ps);
}

size_t
_mb_cur_max_l(locale_t loc)
{
	return MB_CUR_MAX;
}

wint_t
fgetwc(FILE *stream)
{
	return fgetc(stream);
}

wint_t
fputwc(wchar_t wc, FILE *stream)
{
	return fputc(wc & 0xFF, stream);
}
 
wint_t __fputwc_unlock(wchar_t wc, FILE *stream);
wint_t
__fputwc_unlock(wchar_t wc, FILE *stream)
{
	return __sputc(wc & 0xFF, stream);
}

#define	MAPSINGLE(CT)	\
	int	\
	isw##CT(wint_t wc)	\
	{	\
		return is##CT(wc & 0xFF);	\
	}

MAPSINGLE(alnum)
MAPSINGLE(alpha)
MAPSINGLE(blank)
MAPSINGLE(cntrl)
MAPSINGLE(digit)
MAPSINGLE(graph)
MAPSINGLE(lower)
MAPSINGLE(print)
MAPSINGLE(punct)
MAPSINGLE(space)
MAPSINGLE(upper)
MAPSINGLE(xdigit)

int
iswspace_l(wint_t wc, locale_t loc)
{
	return iswspace(wc);
}

struct wct_entry_hack {
	const char *name;
	int (*predicate)(wint_t);
};

#define	WCTENTRY(T)	{ .name= #T , .predicate= isw##T },
static const struct wct_entry_hack my_wcts[] = {
	{ .name = NULL },
	WCTENTRY(alnum)
	WCTENTRY(alpha)
	WCTENTRY(blank)
	WCTENTRY(cntrl)
	WCTENTRY(digit)
	WCTENTRY(graph)
	WCTENTRY(lower)
	WCTENTRY(print)
	WCTENTRY(punct)
	WCTENTRY(space)
	WCTENTRY(upper)
	WCTENTRY(xdigit)
};

wctype_t
wctype(const char *charclass)
{

	for (size_t i = 1; i < __arraycount(my_wcts); i++)
		if (strcmp(charclass, my_wcts[i].name) == 0)
			return (wctype_t)i;

	return (wctype_t)0;
}

int
iswctype(wint_t wc, wctype_t charclass)
{
	size_t ndx = (size_t)charclass;

	if (ndx < 1 || ndx >= __arraycount(my_wcts))
		return 0;

	return my_wcts[ndx].predicate(wc);
}

size_t
wcslen(const wchar_t *s)
{
	size_t l;

	if (s == NULL)
		return 0;

	for (l = 0; *s; l++)
		s++;

	return l;
}

int
wcswidth(const wchar_t *pwcs, size_t n)
{
	int cols;

	if (pwcs == NULL)
		return 0;

	if (*pwcs == 0)
		return 0;

	for (cols = 0; *pwcs && n > 0; cols++)
		if (!isprint(*pwcs & 0xFF))
			return -1;
	return cols;
}

int
wcwidth(wchar_t wc)
{
	if (wc == 0)
		return 0;
	if (!isprint(wc & 0xFF))
		return -1;
	return 1;
}

wchar_t *
wmemchr(const wchar_t *s, wchar_t c, size_t n)
{

	if (s == NULL)
		return NULL;
	while (*s != 0 && *s != c)
		s++;
	if (*s != 0)
		return __UNCONST(s);
	return NULL;
}

wchar_t *
wmemcpy(wchar_t * restrict s1, const wchar_t * restrict s2, size_t n)
{
	wchar_t *p;

	for (p = s1; n > 0; n--)
		*p++ = *s2++;

	return s1;
}

wint_t
towlower(wint_t wc)
{
	return tolower(wc & 0xFF);
}

wint_t
towupper(wint_t wc)
{
	return toupper(wc & 0xFF);
}


