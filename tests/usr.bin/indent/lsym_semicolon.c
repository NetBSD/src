/* $NetBSD: lsym_semicolon.c,v 1.4 2022/04/24 10:36:37 rillig Exp $ */

/*
 * Tests for the token lsym_semicolon, which represents ';' in these contexts:
 *
 * In a declaration, ';' finishes the declaration.
 *
 * In a statement, ';' finishes the statement.
 *
 * In a 'for' statement, ';' separates the 3 expressions in the head of the
 * 'for' statement.
 */

//indent input
struct {
	int member;
} global_var;
//indent end

//indent run-equals-input -di0


//indent input
void
function(void)
{
	for ( ; ; )
		stmt();
	for (;;)
		stmt();
}
//indent end

//indent run
void
function(void)
{
	for (;;)
		stmt();
	for (;;)
		stmt();
}
//indent end
