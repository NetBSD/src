/*	$NetBSD: d_lint_assert.c,v 1.2 2021/01/10 18:13:43 rillig Exp $	*/
# 3 "d_lint_assert.c"

/*
 * Trigger the various assertions in the lint1 code.  Several of them are
 * just hard to trigger, but not impossible.
*/

enum {
	// Before decl.c 1.118 from 2021-01-10:
	// lint: assertion "sym->s_scl == EXTERN || sym->s_scl == STATIC"
	// failed in check_global_variable at decl.c:3135
	// near d_lint_assert.c:14
	A = +++
};
