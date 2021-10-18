/* $NetBSD: token_stmt_list.c,v 1.1 2021/10/18 22:30:34 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for lists of statements.
 *
 * Since C99, in such a statement list, statements can be intermixed with
 * declarations in arbitrary ways.
 */

#indent input
void
function(void)
{
	stmt();
	int var;
	stmt();
	{
		stmt();
		int var;
		stmt();
	}
}
#indent end

#indent run-equals-input -ldi0
