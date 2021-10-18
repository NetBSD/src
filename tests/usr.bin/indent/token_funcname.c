/* $NetBSD: token_funcname.c,v 1.1 2021/10/18 22:30:34 rillig Exp $ */
/* $FreeBSD$ */

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
