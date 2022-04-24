/* $NetBSD: psym_stmt.c,v 1.4 2022/04/24 10:36:37 rillig Exp $ */

/*
 * Tests for the parser symbol psym_stmt, which represents a statement on the
 * stack.
 *
 * TODO: Explain why the stack contains 'lbrace' 'stmt' instead of only 'lbrace'.
 */

//indent input
#define unless(cond) if (!(cond))

void
function(void)
{
	stmt();
	stmt;			/* probably some macro */

	unless(cond)
		stmt();
}
//indent end

/*
 * There is no space after 'unless' since indent cannot know that it is a
 * syntactic macro, especially not when its definition is in a header file.
 */
//indent run-equals-input
