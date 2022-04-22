/* $NetBSD: token_switch_expr.c,v 1.2 2022/04/22 21:21:20 rillig Exp $ */

/*
 * Tests for the keyword 'switch' followed by a parenthesized expression.
 */

#indent input
void
function(void)
{
	switch (expr) {
	}
}
#indent end

#indent run-equals-input
