/* $NetBSD: token_do_stmt.c,v 1.1 2021/10/18 22:30:34 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for do-while statements.
 */

#indent input
void function(void) {
	do stmt(); while (0);
	do { stmt(); } while (0);
	do /* comment */ stmt(); while (0);
}
#indent end

#indent run
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
#indent end
