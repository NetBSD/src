/*	$NetBSD: msg_143.c,v 1.5 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_143.c"

// Test for message: cannot take size/alignment of incomplete type [143]

/* lint1-extra-flags: -X 351 */

/* expect+1: warning: struct 'incomplete' never defined [233] */
struct incomplete;

unsigned long
sizeof_incomplete(void)
{
	/* expect+1: error: cannot take size/alignment of incomplete type [143] */
	return sizeof(struct incomplete);
}
