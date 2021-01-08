/*	$NetBSD: msg_056.c,v 1.2 2021/01/08 21:25:03 rillig Exp $	*/
# 3 "msg_056.c"

// Test for message: integral constant too large [56]

enum color {
	WHITE = 0xFFFFFFFFFFFFFFFFFFFF
};
