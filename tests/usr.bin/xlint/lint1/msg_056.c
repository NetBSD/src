/*	$NetBSD: msg_056.c,v 1.4 2022/06/15 20:18:31 rillig Exp $	*/
# 3 "msg_056.c"

// Test for message: integral constant too large [56]

enum color {
	/* expect+1: warning: integer constant out of range [252] */
	WHITE = 0xFFFFFFFFFFFFFFFFFFFF
};
/* expect-1: warning: integral constant too large [56] */
