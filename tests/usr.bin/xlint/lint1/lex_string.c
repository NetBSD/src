/*	$NetBSD: lex_string.c,v 1.1 2021/06/19 08:30:08 rillig Exp $	*/
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

	/* expect+5: unknown character \134 */
	/* expect+4: syntax error '0' */
	/* expect+3: unknown character \134 */
	/* expect+2: unknown character \134 */
	/* expect+1: unknown character \134 */
	sink("\0\0\0\0");

	/* expect+1: unknown character \134 */
	sink("\x");		/* unfinished */

	/* expect+1: unknown character \134 */
	sink("\y");		/* unknown escape sequence */
}

/* expect+1: cannot recover from previous errors */
