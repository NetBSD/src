/*	$NetBSD: msg_094.c,v 1.6 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_094.c"

// Test for message: function '%s' has illegal storage class [94]

/* lint1-extra-flags: -X 351 */

/* expect+2: error: illegal storage class [8] */
register int
global_example(int arg)
{
	/* expect+1: error: function 'register_example' has illegal storage class [94] */
	register int register_example(int);

	return arg;
}
