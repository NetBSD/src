/* $NetBSD: psym_switch_expr.c,v 1.4 2022/04/24 09:04:12 rillig Exp $ */

/*
 * Tests for the parser symbol psym_switch_expr, which represents the keyword
 * 'switch' followed by the controlling expression, now waiting for a
 * statement (usually a block) containing the 'case' labels.
 */

//indent input
// TODO: add input
//indent end

//indent run-equals-input


/*
 * In all practical cases, a 'switch (expr)' is followed by a block, but the
 * C syntax allows an arbitrary statement.  Unless such a statement has a
 * label, it is unreachable.
 */
//indent input
void
function(void)
{
	switch (expr)
	if (cond) {
	case 1: return;
	case 2: break;
	}
}
//indent end

//indent run
void
function(void)
{
	switch (expr)
		if (cond) {
	case 1:
			return;
	case 2:
			break;
		}
}
//indent end
