/* $NetBSD: psym_while_expr.c,v 1.4 2022/04/24 09:04:12 rillig Exp $ */

/*
 * Tests for the parser symbol psym_while_expr, which represents the keyword
 * 'while' followed by the controlling expression, now waiting for the
 * statement of the loop body.
 */

//indent input
// TODO: add input
//indent end

//indent run-equals-input


//indent input
void
function(void)
{
	while(cond){}

	do{}while(cond);

	if(cmd)while(cond);

	{}while(cond);
}
//indent end

//indent run
void
function(void)
{
	while (cond) {
	}

	do {
	} while (cond);

	if (cmd)
	/* $ XXX: Where does the code say that ';' stays on the same line? */
		while (cond);

	{
	/* $ FIXME: the '}' must be on a line of its own. */
	} while (cond);
}
//indent end
