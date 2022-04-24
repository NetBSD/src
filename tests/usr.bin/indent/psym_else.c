/* $NetBSD: psym_else.c,v 1.4 2022/04/24 10:36:37 rillig Exp $ */

/*
 * Tests for the parser symbol psym_else, which represents the keyword 'else'
 * that is being shifted on the parser stack.
 *
 * This parser symbol never ends up on the stack itself.
 */

/*
 * When parsing nested incomplete 'if' statements, the problem of the
 * 'dangling else' occurs.  It is resolved by binding the 'else' to the
 * innermost incomplete 'if' statement.
 *
 * In 'parse', an if_expr_stmt is reduced to a simple statement, unless the
 * next token is 'else'. The comment does not influence this since it never
 * reaches 'parse'.
 */
//indent input
void
example(bool cond)
{
	if (cond)
	if (cond)
	if (cond)
	stmt();
	else
	stmt();
	/* comment */
	else
	stmt();
}
//indent end

//indent run
void
example(bool cond)
{
	if (cond)
		if (cond)
			if (cond)
				stmt();
			else
				stmt();
	/* comment */
		else
			stmt();
}
//indent end


/*
 * The keyword 'else' is followed by an expression, as opposed to 'if', which
 * is followed by a parenthesized expression.
 */
//indent input
void
function(void)
{
	if(var>0)var=0;else(var=3);
}
//indent end

//indent run
void
function(void)
{
	if (var > 0)
		var = 0;
	else
		(var = 3);
}
//indent end
