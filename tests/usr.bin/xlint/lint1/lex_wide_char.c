/*	$NetBSD: lex_wide_char.c,v 1.4 2023/03/28 14:44:34 rillig Exp $	*/
# 3 "lex_wide_char.c"

/*
 * Tests for lexical analysis of character constants.
 *
 * C99 6.4.4.4 "Character constants"
 */

/* lint1-extra-flags: -X 351 */

void sink(int);

void
test(void)
{
	/* expect+1: error: empty character constant [73] */
	sink(L'');

	sink(L'a');

	sink(L'\0');

	/* UTF-8 */
	/* expect+1: error: too many characters in character constant [71] */
	sink(L'ä');

	/* GCC extension */
	/* expect+1: warning: dubious escape \e [79] */
	sink(L'\e');

	/* since C99 */
	sink(L'\x12');

	/* octal */
	sink(L'\177');

	/* newline */
	sink(L'\n');

	/* expect+1: error: empty character constant [73] */
	sink(L'');
}
