/*	$NetBSD: msg_333.c,v 1.1 2021/01/14 22:18:14 rillig Exp $	*/
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
	if (0)
		return "constant int or bool";
	return p + i;
}
