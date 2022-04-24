/* $NetBSD: fmt_expr.c,v 1.4 2022/04/24 09:04:12 rillig Exp $ */

/*
 * Tests for all kinds of expressions that are not directly related to unary
 * or binary operators.
 *
 * See also:
 *	lsym_binary_op.c
 *	lsym_unary_op.c
 */

/* See lsym_offsetof.c. */
//indent input
void t(void) {
    int n = malloc(offsetof(struct s, f) + 1);
}
//indent end

//indent run
void
t(void)
{
	int		n = malloc(offsetof(struct s, f) + 1);
}
//indent end
