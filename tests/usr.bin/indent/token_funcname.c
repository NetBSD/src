/* $NetBSD: token_funcname.c,v 1.3 2022/04/24 09:04:12 rillig Exp $ */

/*
 * Tests for function names.
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
