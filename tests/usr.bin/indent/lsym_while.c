/* $NetBSD: lsym_while.c,v 1.6 2023/06/02 15:07:46 rillig Exp $ */

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


/*
 * The keyword 'while' must only be indented if it follows a psym_do_stmt,
 * otherwise it starts a new statement and must start a new line.
 */
//indent input
void
function(void)
{
	{} while (0);
}
//indent end

//indent run
void
function(void)
{
	{
	}
	while (0);
}
//indent end
