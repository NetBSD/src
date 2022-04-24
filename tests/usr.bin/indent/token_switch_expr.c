/* $NetBSD: token_switch_expr.c,v 1.3 2022/04/24 09:04:12 rillig Exp $ */

/*
 * Tests for the keyword 'switch' followed by a parenthesized expression.
 */

//indent input
void
function(void)
{
	switch (expr) {
	}
}
//indent end

//indent run-equals-input
