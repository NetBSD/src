/* $NetBSD: psym_do_stmt.c,v 1.5 2025/01/07 03:55:00 rillig Exp $ */

/*
 * Tests for the parser symbol psym_do_stmt, which represents the state after
 * reading the keyword 'do' and the loop body, now waiting for the keyword
 * 'while' and the controlling expression.
 */

//indent input
void function(void) {
	do stmt(); while (0);
	do { stmt(); } while (0);
	do /* comment */ stmt(); while (0);
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
		stmt();
	} while (0);
	do			/* comment */
		stmt();
	while (0);
}
//indent end


//indent input
{
	if (cond) do stmt; while (cond); stmt;
}
//indent end

//indent run
{
	if (cond)
		do
			stmt;
		while (cond);
	//$ Ensure that this statement is indented the same as the 'if' above.
	stmt;
}
//indent end
