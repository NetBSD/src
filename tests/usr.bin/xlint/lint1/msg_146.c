/*	$NetBSD: msg_146.c,v 1.2 2021/01/08 21:25:03 rillig Exp $	*/
# 3 "msg_146.c"

// Test for message: cannot take size/alignment of void [146]

void
example(void *ptr)
{
	return sizeof(*ptr);
}
