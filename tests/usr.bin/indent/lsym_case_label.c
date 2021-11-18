/* $NetBSD: lsym_case_label.c,v 1.1 2021/11/18 21:19:19 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for the token lsym_case_label, which represents either the keyword
 * 'case' or the keyword 'default', which are both used in 'switch'
 * statements.
 *
 * Since C11, the keyword 'default' is used in _Generic selections as well.
 *
 * See also:
 *	psym_switch_expr.c
 *	C11 6.5.1.1		"Generic selection"
 */

// TODO: test C11 _Generic

#indent input
// TODO: add input
#indent end

#indent run-equals-input
