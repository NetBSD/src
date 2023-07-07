/*	$NetBSD: msg_109.c,v 1.6 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_109.c"

// Test for message: void type illegal in expression [109]

/* lint1-extra-flags: -X 351 */

/* ARGSUSED */
int
example(int arg)
{
	/* expect+2: error: void type illegal in expression [109] */
	/* expect+1: warning: function 'example' expects to return value [214] */
	return arg + (void)4;
}
