/* $NetBSD: lsym_if.c,v 1.4 2022/04/24 10:36:37 rillig Exp $ */

/*
 * Tests for the token lsym_if, which represents the keyword 'if' that starts
 * an 'if' or 'if-else' statement.
 */

//indent input
void
function(void)
{
	if(cond)stmt();
}
//indent end

//indent run
void
function(void)
{
	if (cond)
		stmt();
}
//indent end
