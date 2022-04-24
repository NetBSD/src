/* $NetBSD: psym_do.c,v 1.4 2022/04/24 10:36:37 rillig Exp $ */

/*
 * Tests for the parser symbol psym_do, which represents the state after
 * reading the token 'do', now waiting for the statement of the loop body.
 */

//indent input
void function(void) {
	do stmt(); while (0);
	do {} while (0);
}
//indent end

//indent run
void
function(void)
{
	do
		stmt();
	while (0);
	do {
	} while (0);
}
//indent end


/*
 * The keyword 'do' is followed by a statement, as opposed to 'while', which
 * is followed by a parenthesized expression.
 */
//indent input
void
function(void)
{
	do(var)--;while(var>0);
}
//indent end

//indent run
void
function(void)
{
	do
		(var)--;
	while (var > 0);
}
//indent end
