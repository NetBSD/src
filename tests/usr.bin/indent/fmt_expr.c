/* $NetBSD: fmt_expr.c,v 1.9 2023/06/16 12:55:57 rillig Exp $ */

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

	for (ln = gnodes->first; ln != NULL; ln = ln->next)
		*(GNode **)Vector_Push(&vec) = ln->datum;
}
//indent end

//indent run-equals-input


/*
 * GCC statement expressions are not supported yet.
 */
//indent input
{
	int var = ({1});
	int var = ({
		1
	});
	int var = ({
		int decl = 1;
		stmt;
	});
}
//indent end

//indent run -di0
{
	int var = ({1});
	int var = ({
		   1
		   });
	int var = ({
		   int decl = 1;
		stmt;
	});
}
exit 1
error: Standard Input:7: Unbalanced parentheses
warning: Standard Input:9: Extra ')'
//indent end
