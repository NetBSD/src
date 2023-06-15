/* $NetBSD: lsym_funcname.c,v 1.6 2023/06/15 09:19:07 rillig Exp $ */

/*
 * Tests for the token lsym_funcname, which is the name of a function, but only
 * in a function definition, not in a declaration or a call expression.
 *
 * See also:
 *	lsym_word.c
 */

//indent input
void
function(void)
{
	func();
	(func)();
	func(1, 2, 3);
}
//indent end

//indent run-equals-input


/*
 * The comment after the return type of a function definition is a code
 * comment, not a declaration comment.
 */
//indent input
void				// comment
function_with_comment(void)
{
}
//indent end

//indent run-equals-input
