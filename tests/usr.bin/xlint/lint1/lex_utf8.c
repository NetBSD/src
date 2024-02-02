/*	$NetBSD: lex_utf8.c,v 1.1 2024/02/02 23:30:39 rillig Exp $	*/
# 3 "lex_utf8.c"

/*
 * Test lexing of multibyte characters and strings in an UTF-8 locale.
 */

/*
 * Since lint always runs in the default "C" locale, it does not support any
 * multibyte character encoding, thus treating each byte as a separate
 * character. If lint were to support UTF-8, the array dimension would be 3
 * instead of 7.
 */
/* expect+1: error: negative array dimension (-7) [20] */
typedef int mblen[-(int)(sizeof(L"Ä😄") / sizeof(L""))];
