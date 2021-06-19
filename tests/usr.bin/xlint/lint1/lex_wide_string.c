/*	$NetBSD: lex_wide_string.c,v 1.1 2021/06/19 08:30:08 rillig Exp $	*/
# 3 "lex_wide_string.c"

/*
 * Test lexical analysis of wide string constants.
 *
 * C99 6.4.5 "String literals"
 */

void sink(const int *);

void
test(void)
{
	sink(L"");

	sink(L"hello, world\n");

	sink(L"\0");

	sink(L"\0\0\0\0");

	/* expect+1: no hex digits follow \x [74] */
	sink(L"\x");

	/* expect+1: dubious escape \y [79] */
	sink(L"\y");
}
