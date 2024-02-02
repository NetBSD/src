/*	$NetBSD: lex_utf8.c,v 1.2 2024/02/02 23:36:01 rillig Exp $	*/
# 3 "lex_utf8.c"

/*
 * Test lexing of multibyte characters and strings in an UTF-8 locale.
 *
 * See also:
 *	lex_wide_string.c	(runs in the C locale)
 */

/* expect+1: error: negative array dimension (-3) [20] */
typedef int mblen[-(int)(sizeof(L"Ä😄") / sizeof(L""))];
