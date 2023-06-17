/* $NetBSD: lsym_funcname.c,v 1.7 2023/06/17 22:09:24 rillig Exp $ */

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


/*
 * The heuristics for telling a function definition and a function declaration
 * apart look at the remaining characters in a line but don't tokenize them.
 * Due to that, a ');' in a comment influences the heuristics.
 */
//indent input
// $ This ');' in the comment does not mark the end of the declaration.
void heuristics_semicolon_comment(/* ); */) {}
void heuristics_semicolon_no_comm(/* -- */) {}
void heuristics_comma_comment(/* ), */) {}
void heuristics_comma_no_comm(/* -- */) {}
//indent end

//indent run -di0
void heuristics_semicolon_comment(/* ); */) {
}
void
heuristics_semicolon_no_comm(/* -- */)
{
}
void heuristics_comma_comment(/* ), */) {
}
void
heuristics_comma_no_comm(/* -- */)
{
}
//indent end
