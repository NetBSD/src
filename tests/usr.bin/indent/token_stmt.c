/* $NetBSD: token_stmt.c,v 1.3 2022/04/24 09:04:12 rillig Exp $ */

/*
 * Tests for statements.
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
