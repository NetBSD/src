/*	$NetBSD: d_lint_assert.c,v 1.5 2022/01/15 14:22:03 rillig Exp $	*/
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
/* expect-1: error: syntax error '}' [249] */

/*
 * Before decl.c 1.196 from 2021-07-10, lint ran into an assertion failure
 * for 'sym->s_type != NULL' in declare_argument.
 */
/* expect+1: warning: old style declaration; add 'int' [1] */
c(void());
