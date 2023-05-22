/* $NetBSD: lsym_funcname.c,v 1.5 2023/05/22 23:01:27 rillig Exp $ */

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
