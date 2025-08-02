/*	$NetBSD: msg_094.c,v 1.6.2.1 2025/08/02 05:58:16 perseant Exp $	*/
# 3 "msg_094.c"

// Test for message: function '%s' has invalid storage class [94]

/* lint1-extra-flags: -X 351 */

/* expect+2: error: invalid storage class [8] */
register int
global_example(int arg)
{
	/* expect+1: error: function 'register_example' has invalid storage class [94] */
	register int register_example(int);

	return arg;
}
