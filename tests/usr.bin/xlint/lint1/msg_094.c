/*	$NetBSD: msg_094.c,v 1.4 2022/04/03 09:34:45 rillig Exp $	*/
# 3 "msg_094.c"

// Test for message: function has illegal storage class: %s [94]

/* expect+2: error: illegal storage class [8] */
register int
global_example(int arg)
{
	/* expect+1: error: function has illegal storage class: register_example [94] */
	register int register_example(int);

	return arg;
}
