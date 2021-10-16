/* $NetBSD: opt_pcs.c,v 1.2 2021/10/16 05:40:17 rillig Exp $ */
/* $FreeBSD$ */

#indent input
void
example(void)
{
	function_call();
	function_call(1);
	function_call(1,2,3);
}
#indent end

#indent run -pcs
void
example(void)
{
	function_call ();
	function_call (1);
	function_call (1, 2, 3);
}
#indent end

#indent run -npcs
void
example(void)
{
	function_call();
	function_call(1);
	function_call(1, 2, 3);
}
#indent end
