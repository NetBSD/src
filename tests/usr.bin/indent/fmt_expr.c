/* $NetBSD: fmt_expr.c,v 1.5 2023/06/09 09:45:55 rillig Exp $ */

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


//indent input
{
	for (ln = gnodes->first; ln != NULL; ln = ln->next)
// $ FIXME: No space after the cast.
		*(GNode **) Vector_Push(&vec) = ln->datum;
}
//indent end

//indent run-equals-input
