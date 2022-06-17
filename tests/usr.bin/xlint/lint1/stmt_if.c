/*	$NetBSD: stmt_if.c,v 1.2 2022/06/17 18:54:53 rillig Exp $	*/
# 3 "stmt_if.c"

/*
 * Test parsing of 'if' statements.
 */

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
