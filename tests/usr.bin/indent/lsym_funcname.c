/* $NetBSD: lsym_funcname.c,v 1.4 2022/04/24 10:36:37 rillig Exp $ */

/*
 * Tests for the token lsym_funcname, which is an identifier that is followed
 * by an opening parenthesis.
 *
 * TODO: Document how lsym_funcname is handled differently from lsym_word.
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
