/*	$NetBSD: lex_integer_binary.c,v 1.2 2022/06/17 18:54:53 rillig Exp $	*/
# 3 "lex_integer_binary.c"

/*
 * Test for parsing binary integer literals in non-GCC mode.
 * As of C11, binary integer literals are not supported.
 * Neither are underscores in integer literals.
 */

/* Remove the default -g flag. */
/* lint1-flags: -Ac11 -w */

void sink(unsigned int);

void
binary_literal(void)
{
	/*
	 * Binary integer literals are a GCC extension, but lint allows them
	 * even in non-GCC mode.
	 */
	sink(0b1111000001011010);

	/*
	 * Even though it would be useful for binary literals, GCC does not
	 * support underscores to separate the digit groups.
	 */
	/* expect+1: error: syntax error '_0000_0101_1010' [249] */
	sink(0b1111_0000_0101_1010);
}
