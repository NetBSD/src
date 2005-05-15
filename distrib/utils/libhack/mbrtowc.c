/*      $NetBSD: mbrtowc.c,v 1.1 2005/05/15 10:15:47 dsl Exp $      */

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
