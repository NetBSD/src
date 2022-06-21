/*	$NetBSD: msg_094.c,v 1.5 2022/06/21 21:18:30 rillig Exp $	*/
# 3 "msg_094.c"

// Test for message: function '%s' has illegal storage class [94]

/* expect+2: error: illegal storage class [8] */
register int
global_example(int arg)
{
	/* expect+1: error: function 'register_example' has illegal storage class [94] */
	register int register_example(int);

	return arg;
}
