/*	$NetBSD: stmt_if.c,v 1.1 2021/07/11 18:58:13 rillig Exp $	*/
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
	/* expect+1: syntax error 'else' [249] */
	else
		println("syntax error");
}
