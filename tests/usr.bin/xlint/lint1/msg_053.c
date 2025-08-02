/*	$NetBSD: msg_053.c,v 1.8.2.1 2025/08/02 05:58:15 perseant Exp $	*/
# 3 "msg_053.c"

// Test for message: declared parameter '%s' is missing [53]

/* lint1-extra-flags: -X 351 */

/* expect+1: warning: function definition for 'oldstyle' with identifier list is obsolete in C23 [384] */
oldstyle(parameter)
	/* expect+1: error: old-style declaration; add 'int' [1] */
	int parameter;
	/* expect+1: error: declared parameter 'extra_parameter' is missing [53] */
	int extra_parameter;
{
	return parameter;
}
