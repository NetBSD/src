/*	$NetBSD: lex_string.c,v 1.6 2023/03/28 14:44:34 rillig Exp $	*/
# 3 "lex_string.c"

/*
 * Test lexical analysis of string constants.
 *
 * C99 6.4.5 "String literals"
 */

/* lint1-extra-flags: -X 351 */

void sink(const char *);

void
test(void)
{
	sink("");

	sink("hello, world\n");

	sink("\0");

	sink("\0\0\0\0");

	/* expect+1: error: no hex digits follow \x [74] */
	sink("\x");		/* unfinished */

	/* expect+1: warning: dubious escape \y [79] */
	sink("\y");		/* unknown escape sequence */

	sink("first" "second");

	/* expect+1: error: cannot concatenate wide and regular string literals [292] */
	sink("plain" L"wide");
}

/* TODO: test digraphs inside string literals */
/* TODO: test trigraphs inside string literals */
