/*	$NetBSD: msg_109.c,v 1.7 2023/08/06 19:44:50 rillig Exp $	*/
# 3 "msg_109.c"

// Test for message: void type illegal in expression [109]

/* lint1-extra-flags: -X 351 */

/* ARGSUSED */
int
example(int arg)
{
	/* expect+2: error: void type illegal in expression [109] */
	/* expect+1: error: function 'example' expects to return value [214] */
	return arg + (void)4;
}
