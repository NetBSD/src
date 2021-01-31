/*	$NetBSD: msg_094.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_094.c"

// Test for message: function has illegal storage class: %s [94]

register int
global_example(int arg)				/* expect: 8 */
{
	register int register_example(int);	/* expect: 94 */

	return arg;
}
