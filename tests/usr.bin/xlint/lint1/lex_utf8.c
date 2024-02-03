/*	$NetBSD: lex_utf8.c,v 1.3 2024/02/03 10:56:18 rillig Exp $	*/
# 3 "lex_utf8.c"

/*
 * Test lexing of multibyte characters and strings in a UTF-8 locale.
 *
 * See also:
 *	lex_wide_string.c	(runs in the C locale)
 */

/* expect+1: error: negative array dimension (-3) [20] */
typedef int mblen[-(int)(sizeof(L"Ä😄") / sizeof(L""))];
