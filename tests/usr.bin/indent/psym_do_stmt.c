/* $NetBSD: psym_do_stmt.c,v 1.4 2022/04/24 10:36:37 rillig Exp $ */

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
