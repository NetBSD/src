/* $NetBSD: token_do_stmt.c,v 1.3 2022/04/24 09:04:12 rillig Exp $ */

/*
 * Tests for do-while statements.
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
