/*	$NetBSD: stmt_goto.c,v 1.2 2022/06/17 18:54:53 rillig Exp $	*/
# 3 "stmt_goto.c"

/*
 * Tests for the 'goto' statement.
 */

/* expect+1: error: syntax error 'goto' [249] */
goto invalid_at_top_level;

void
function(void)
{
	goto label;
label:
	/* expect+1: error: syntax error '"' [249] */
	goto "string";

	/* Reset the error handling of the parser. */
	goto ok;
ok:

	/* Numeric labels work in Pascal, but not in C. */
	/* expect+1: error: syntax error '12345' [249] */
	goto 12345;
}
