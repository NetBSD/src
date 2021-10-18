/* $NetBSD: token_keyword_do.c,v 1.1 2021/10/18 22:30:34 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for the keyword 'do' that begins a do-while statement.
 */

#indent input
void function(void) {
	do stmt(); while (0);
	do {} while (0);
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
	} while (0);
}
#indent end
