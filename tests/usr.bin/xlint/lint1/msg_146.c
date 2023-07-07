/*	$NetBSD: msg_146.c,v 1.5 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_146.c"

// Test for message: cannot take size/alignment of void [146]

/* lint1-extra-flags: -X 351 */

unsigned long
example(void *ptr)
{
	/* expect+1: error: cannot take size/alignment of void [146] */
	return sizeof(*ptr);
}
