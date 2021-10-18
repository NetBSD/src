/* $NetBSD: token_switch_expr.c,v 1.1 2021/10/18 22:30:34 rillig Exp $ */
/* $FreeBSD$ */

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
