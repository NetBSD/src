/*	$NetBSD: msg_094.c,v 1.2 2021/01/07 00:38:46 rillig Exp $	*/
# 3 "msg_094.c"

// Test for message: function has illegal storage class: %s [94]

register int
global_example(int arg)
{
	register int register_example(int);

	return arg;
}
