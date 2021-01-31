/*	$NetBSD: msg_056.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_056.c"

// Test for message: integral constant too large [56]

enum color {
	WHITE = 0xFFFFFFFFFFFFFFFFFFFF	/* expect: 252 */
};					/* expect: 56 */
