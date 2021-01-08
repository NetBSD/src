/*	$NetBSD: msg_066.c,v 1.2 2021/01/08 21:25:03 rillig Exp $	*/
# 3 "msg_066.c"

// Test for message: syntax requires ';' after last struct/union member [66]

struct number {
	int value
};
