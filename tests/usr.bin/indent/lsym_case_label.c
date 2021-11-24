/* $NetBSD: lsym_case_label.c,v 1.3 2021/11/24 21:34:34 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for the token lsym_case_label, which represents either the keyword
 * 'case' or the keyword 'default', which are both used in 'switch'
 * statements.
 *
 * Since C11, the keyword 'default' is used in _Generic selections as well.
 *
 * See also:
 *	opt_cli.c
 *	psym_switch_expr.c
 *	C11 6.5.1.1		"Generic selection"
 */

// TODO: test C11 _Generic

#indent input
// TODO: add input
#indent end

#indent run-equals-input


/*
 * If there is a '{' after a case label, it gets indented using tabs instead
 * of spaces. Indent does not necessarily insert a space in this situation,
 * which looks strange.
 */
#indent input
void
function(void)
{
	switch (expr) {
	case 1: {
		break;
	}
	case 11: {
		break;
	}
	}
}
#indent end

#indent run
void
function(void)
{
	switch (expr) {
	/* $ The space between the ':' and the '{' is actually a tab. */
	case 1:	{
			break;
		}
	/* $ FIXME: missing space between ':' and '{'. */
	case 11:{
			break;
		}
	}
}
#indent end
