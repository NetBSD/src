/*	$NetBSD: lex_wide_string.c,v 1.4 2023/03/28 14:44:34 rillig Exp $	*/
# 3 "lex_wide_string.c"

/*
 * Test lexical analysis of wide string constants.
 *
 * C99 6.4.5 "String literals"
 */

/* lint1-extra-flags: -X 351 */

void sink(const int *);

void
test(void)
{
	sink(L"");

	sink(L"hello, world\n");

	sink(L"\0");

	sink(L"\0\0\0\0");

	/* expect+1: error: no hex digits follow \x [74] */
	sink(L"\x");

	/* expect+1: warning: dubious escape \y [79] */
	sink(L"\y");

	sink(L"first" L"second");

	/* expect+1: error: cannot concatenate wide and regular string literals [292] */
	sink(L"wide" "plain");
}
