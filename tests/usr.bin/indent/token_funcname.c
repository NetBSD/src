/* $NetBSD: token_funcname.c,v 1.2 2022/04/22 21:21:20 rillig Exp $ */

/*
 * Tests for function names.
 */

#indent input
void
function(void)
{
	func();
	(func)();
	func(1, 2, 3);
}
#indent end

#indent run-equals-input
