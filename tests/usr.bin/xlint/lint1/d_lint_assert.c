/*	$NetBSD: d_lint_assert.c,v 1.8 2023/07/15 09:40:37 rillig Exp $	*/
# 3 "d_lint_assert.c"

/*
 * Trigger the various assertions in the lint1 code.  Several of them are
 * just hard to trigger, but not impossible.
*/

/* lint1-extra-flags: -X 351 */

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
/* expect+1: warning: old-style declaration; add 'int' [1] */
c(void());


// As of 2023-07-15, the following code leads to a crash, due to the word
// 'unknown_type_modifier'.  The parser then goes into error recovery mode and
// discards the declaration in the 'for' loop.  In the end, the symbol table
// still contains symbols that were already freed when parsing the '}' from the
// 'switch' statement.  To reproduce the crash, run 'make -DDEBUG DBG="-O0 -g"'
// and run with -Sy.
//
// static inline void
// f(void)
// {
//	int i = 3;
//
//	for (unknown_type_modifier char *p = "";; ) {
//		switch (i) {
//		case 3:;
//		}
//	}
// }
