/*	$NetBSD: msg_053.c,v 1.9 2024/12/01 18:37:54 rillig Exp $	*/
# 3 "msg_053.c"

// Test for message: declared parameter '%s' is missing [53]

/* lint1-extra-flags: -X 351 */

/* expect+1: warning: function definition with identifier list is obsolete in C23 [384] */
oldstyle(parameter)
	/* expect+1: error: old-style declaration; add 'int' [1] */
	int parameter;
	/* expect+1: error: declared parameter 'extra_parameter' is missing [53] */
	int extra_parameter;
{
	return parameter;
}
