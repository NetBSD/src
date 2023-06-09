/* $NetBSD: psym_stmt.c,v 1.5 2023/06/09 09:45:55 rillig Exp $ */

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


//indent input
{
	TAILQ_FOREACH(a, b, c) {
		a =
// $ FIXME: The 'b' must be indented as a continuation.
// $ The '{' in line 2 sets ps.block_init though, even though it does not
// $ follow a '='.
		b;
	}
}
//indent end

//indent run-equals-input -di0 -nlp -ci4
