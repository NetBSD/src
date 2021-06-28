/*	$NetBSD: msg_215.c,v 1.3 2021/06/28 11:27:00 rillig Exp $	*/
# 3 "msg_215.c"

// Test for message: function implicitly declared to return int [215]

void
caller(void)
{
	callee(12345);		/* expect: [215] */
}
