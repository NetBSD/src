/*	$NetBSD: msg_143.c,v 1.2 2021/01/08 21:25:03 rillig Exp $	*/
# 3 "msg_143.c"

// Test for message: cannot take size/alignment of incomplete type [143]

struct incomplete;

unsigned long
sizeof_incomplete(void)
{
	return sizeof(struct incomplete);
}
