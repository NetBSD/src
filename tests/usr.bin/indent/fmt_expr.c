/* $NetBSD: fmt_expr.c,v 1.1 2021/10/23 20:17:08 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for all kinds of expressions that are not directly related to unary
 * or binary operators.
 *
 * See also: token_binary_op, token_unary_op.
 */

/* See FreeBSD r303718. */
#indent input
void t(void) {
    int n = malloc(offsetof(struct s, f) + 1);
}
#indent end

#indent run
void
t(void)
{
	int		n = malloc(offsetof(struct s, f) + 1);
}
#indent end
