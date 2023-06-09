/* $NetBSD: fmt_expr.c,v 1.6 2023/06/09 11:22:31 rillig Exp $ */

/*
 * Tests for all kinds of expressions that are not directly related to unary
 * or binary operators.
 *
 * See also:
 *	lsym_binary_op.c
 *	lsym_unary_op.c
 */

//indent input
{
	// See lsym_offsetof.c.
	malloc(offsetof(struct s, f) + 1);

	// C99 compound literals use initializer braces.
	println((const char[3]){'-', c, '\0'});
	x = ((struct point){0, 0}).x;

	// XXX: GCC statement expressions are not supported yet.
	int		var =
	(
	 {
	 1
	 }
	)
		       ;

	for (ln = gnodes->first; ln != NULL; ln = ln->next)
// $ FIXME: No space after the cast.
		*(GNode **) Vector_Push(&vec) = ln->datum;
}
//indent end

//indent run-equals-input
