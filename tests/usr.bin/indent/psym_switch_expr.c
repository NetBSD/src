/* $NetBSD: psym_switch_expr.c,v 1.6 2023/06/23 20:44:51 rillig Exp $ */

/*
 * Tests for the parser symbol psym_switch_expr, which represents the keyword
 * 'switch' followed by the controlling expression, now waiting for a
 * statement (usually a block) containing the 'case' labels.
 */

//indent run-equals-input


/*
 * In all practical cases, a 'switch (expr)' is followed by a block, but the
 * C syntax allows an arbitrary statement.  Unless such a statement has a
 * label or is a loop, its beginning is unreachable.
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

//indent run -cli-0.375
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
