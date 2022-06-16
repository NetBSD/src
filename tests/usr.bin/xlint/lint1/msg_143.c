/*	$NetBSD: msg_143.c,v 1.4 2022/06/16 16:58:36 rillig Exp $	*/
# 3 "msg_143.c"

// Test for message: cannot take size/alignment of incomplete type [143]

/* expect+1: warning: struct 'incomplete' never defined [233] */
struct incomplete;

unsigned long
sizeof_incomplete(void)
{
	/* expect+1: error: cannot take size/alignment of incomplete type [143] */
	return sizeof(struct incomplete);
}
