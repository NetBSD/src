/* $NetBSD: psym_stmt.c,v 1.6 2023/06/09 10:24:55 rillig Exp $ */

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


// Ensure that '(something) {' is not treated as a cast expression.
//indent input
{
	TAILQ_FOREACH(a, b, c) {
		a =
		    b;
	}
}
//indent end

//indent run-equals-input -di0 -nlp -ci4
