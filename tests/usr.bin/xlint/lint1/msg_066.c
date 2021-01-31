/*	$NetBSD: msg_066.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_066.c"

// Test for message: syntax requires ';' after last struct/union member [66]

struct number {
	int value
};				/* expect: 66 */
