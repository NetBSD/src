/*	$NetBSD: msg_109.c,v 1.7.2.1 2025/08/02 05:58:16 perseant Exp $	*/
# 3 "msg_109.c"

// Test for message: void type invalid in expression [109]

/* lint1-extra-flags: -X 351 */

/* ARGSUSED */
int
example(int arg)
{
	/* expect+2: error: void type invalid in expression [109] */
	/* expect+1: error: function 'example' expects to return value [214] */
	return arg + (void)4;
}
