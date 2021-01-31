/*	$NetBSD: msg_146.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_146.c"

// Test for message: cannot take size/alignment of void [146]

unsigned long
example(void *ptr)
{
	return sizeof(*ptr);		/* expect: 146 */
}
