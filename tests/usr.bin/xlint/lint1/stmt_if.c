/*	$NetBSD: stmt_if.c,v 1.3 2023/03/28 14:44:35 rillig Exp $	*/
# 3 "stmt_if.c"

/*
 * Test parsing of 'if' statements.
 */

/* lint1-extra-flags: -X 351 */

void println(const char *);

void
dangling_else(int x)
{
	if (x > 0)
		if (x > 10)
			println("> 10");
		/* This 'else' is bound to the closest unfinished 'if'. */
		else
			println("> 0");
	/*
	 * If the above 'else' were bound to the other 'if', the next 'else'
	 * would have no corresponding 'if', resulting in a syntax error.
	 */
	else
		println("not positive");
	/* expect+1: error: syntax error 'else' [249] */
	else
		println("syntax error");
}
