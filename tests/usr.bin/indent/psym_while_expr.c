/* $NetBSD: psym_while_expr.c,v 1.6 2025/01/03 23:37:18 rillig Exp $ */

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
		while (cond)
			;

	{
	}
	while (cond)
		;
}
//indent end
