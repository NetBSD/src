/* $NetBSD: lsym_while.c,v 1.4 2022/04/24 10:36:37 rillig Exp $ */

/*
 * Tests for the token 'lsym_while', which represents the keyword 'while' that
 * starts a 'while' loop or finishes a 'do-while' loop.
 */

//indent input
void
function(void)
{
	while(cond)stmt();
	do stmt();while(cond);
}
//indent end

//indent run
void
function(void)
{
	while (cond)
		stmt();
	do
		stmt();
	while (cond);
}
//indent end
