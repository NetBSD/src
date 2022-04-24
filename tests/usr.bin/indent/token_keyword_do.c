/* $NetBSD: token_keyword_do.c,v 1.3 2022/04/24 09:04:12 rillig Exp $ */

/*
 * Tests for the keyword 'do' that begins a do-while statement.
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
