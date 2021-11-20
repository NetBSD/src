/*	$NetBSD: lex_string.c,v 1.4 2021/11/20 17:27:46 rillig Exp $	*/
# 3 "lex_string.c"

/*
 * Test lexical analysis of string constants.
 *
 * C99 6.4.5 "String literals"
 */

void sink(const char *);

void
test(void)
{
	sink("");

	sink("hello, world\n");

	sink("\0");

	sink("\0\0\0\0");

	/* expect+1: no hex digits follow \x [74] */
	sink("\x");		/* unfinished */

	/* expect+1: dubious escape \y [79] */
	sink("\y");		/* unknown escape sequence */

	sink("first" "second");

	/* expect+1: error: cannot concatenate wide and regular string literals [292] */
	sink("plain" L"wide");
}

/* TODO: test digraphs inside string literals */
/* TODO: test trigraphs inside string literals */
