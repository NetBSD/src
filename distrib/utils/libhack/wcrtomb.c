/*      $NetBSD: wcrtomb.c,v 1.1 2007/02/03 19:55:51 christos Exp $      */

/*
 * Ignore all multibyte sequences, removes all the citrus code.
 * Probably only used by vfprintf() when parsing the format string.
 */

#include <wchar.h>

size_t
wcrtomb(char *str, wchar_t wc, mbstate_t *ps)
{
    *str = wc;
    return 1;
}
