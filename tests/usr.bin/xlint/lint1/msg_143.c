/*	$NetBSD: msg_143.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_143.c"

// Test for message: cannot take size/alignment of incomplete type [143]

struct incomplete;				/* expect: 233 */

unsigned long
sizeof_incomplete(void)
{
	return sizeof(struct incomplete);	/* expect: 143 */
}
