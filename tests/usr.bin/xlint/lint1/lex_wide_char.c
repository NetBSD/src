/*	$NetBSD: lex_wide_char.c,v 1.2 2021/06/20 18:38:12 rillig Exp $	*/
# 3 "lex_wide_char.c"

/*
 * Tests for lexical analysis of character constants.
 *
 * C99 6.4.4.4 "Character constants"
 */

void sink(int);

void
test(void)
{
	/* expect+1: empty character constant */
	sink(L'');

	sink(L'a');

	sink(L'\0');

	/* UTF-8 */
	/* expect+1: too many characters in character constant */
	sink(L'ä');

	/* GCC extension */
	/* expect+1: dubious escape \e */
	sink(L'\e');

	/* since C99 */
	sink(L'\x12');

	/* octal */
	sink(L'\177');

	/* newline */
	sink(L'\n');

	/* expect+1: empty character constant */
	sink(L'');
}
