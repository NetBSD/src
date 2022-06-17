/*	$NetBSD: gcc_stmt_asm.c,v 1.4 2022/06/17 18:54:53 rillig Exp $	*/
# 3 "gcc_stmt_asm.c"

/*
 * Tests for the GCC 'asm' statement.
 */

void
function(void)
{
	/*
	 * lint is not really interested in assembly language, therefore it
	 * just skips everything until and including the closing parenthesis.
	 */
	asm(any "string" or 12345 || whatever);

	/*
	 * Parentheses are allowed in 'asm' statements, they have to be
	 * properly nested.  Brackets and braces don't have to be nested
	 * since they usually not occur in 'asm' statements.
	 */
	__asm(^(int = typedef[[[{{{));

	__asm__();
}

/*
 * Even on the top level, 'asm' is allowed.  It is interpreted as a
 * declaration.
 */
__asm__();

void
syntax_error(void)
{
	/* expect+1: error: syntax error '__asm__' [249] */
	int i = __asm__();
}

__asm__(
/* cover read_until_rparen at EOF */
/* expect+1: error: syntax error '' [249] */
