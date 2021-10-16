/* $NetBSD: opt_pcs.c,v 1.3 2021/10/16 21:32:10 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for the options '-pcs' and '-npcs'.
 *
 * The option '-pcs' adds a space in a function call expression, between the
 * function name and the opening parenthesis.
 *
 * The option '-npcs' removes any whitespace from a function call expression,
 * between the function name and the opening parenthesis.
 */

#indent input
void
example(void)
{
	function_call();
	function_call (1);
	function_call   (1,2,3);
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
