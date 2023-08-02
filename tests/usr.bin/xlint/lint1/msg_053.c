/*	$NetBSD: msg_053.c,v 1.8 2023/08/02 18:51:25 rillig Exp $	*/
# 3 "msg_053.c"

// Test for message: declared parameter '%s' is missing [53]

/* lint1-extra-flags: -X 351 */

/* expect+2: error: old-style declaration; add 'int' [1] */
oldstyle(parameter)
	int parameter;
	/* expect+1: error: declared parameter 'extra_parameter' is missing [53] */
	int extra_parameter;
{
	return parameter;
}
