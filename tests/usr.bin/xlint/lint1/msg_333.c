/*	$NetBSD: msg_333.c,v 1.3 2021/03/21 14:36:59 rillig Exp $	*/
# 3 "msg_333.c"

// Test for message: controlling expression must be bool, not '%s' [333]
//
// See d_c99_bool_strict.c for many more examples.

/* lint1-extra-flags: -T */

typedef _Bool bool;

const char *
example(bool b, int i, const char *p)
{
	if (b)
		return "bool";
	if (i)			/* expect: 333 */
		return "int";
	if (p)			/* expect: 333 */
		return "pointer";
	if (__lint_false)
		return "bool constant"; /* expect: statement not reached */
	if (0)			/* expect: 333 */
		return "integer constant";
	return p + i;
}
