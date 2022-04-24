/* $NetBSD: psym_stmt_list.c,v 1.4 2022/04/24 10:36:37 rillig Exp $ */

/*
 * Tests for the parser symbol psym_stmt_list, which represents a list of
 * statements.
 *
 * Since C99, in such a statement list, statements can be intermixed with
 * declarations.
 *
 * TODO: explain why psym_stmt and psym_stmt_list are both necessary.
 */

//indent input
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
//indent end

//indent run-equals-input -ldi0


//indent input
void
return_after_rbrace(void)
{
	{}return;
}
//indent end

//indent run
void
return_after_rbrace(void)
{
	{
// $ FIXME: The 'return' must go in a separate line.
	} return;
}
//indent end
