/* $NetBSD: psym_if_expr.c,v 1.5 2023/06/10 17:56:29 rillig Exp $ */

/*
 * Tests for the parser symbol psym_if_expr, representing the parser state
 * after reading the keyword 'if' and the controlling expression, now waiting
 * for the statement of the 'then' branch.
 */

//indent input
void function(void) {
	if(cond) stmt();
}
//indent end

//indent run
void
function(void)
{
	if (cond)
		stmt();
}
//indent end


/*
 * Indent is forgiving about syntax errors such as an 'if' statement in which
 * the condition is not parenthesized.
 */
//indent input
{
	if cond {
	}
	if cond && cond {
	}
}
//indent end

//indent run
{
	if cond {
	}
	if cond
		&& cond {
		}
}
//indent end
