/*	$NetBSD: msg_146.c,v 1.4 2022/06/16 16:58:36 rillig Exp $	*/
# 3 "msg_146.c"

// Test for message: cannot take size/alignment of void [146]

unsigned long
example(void *ptr)
{
	/* expect+1: error: cannot take size/alignment of void [146] */
	return sizeof(*ptr);
}
