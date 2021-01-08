/*	$NetBSD: msg_055.c,v 1.2 2021/01/08 21:25:03 rillig Exp $	*/
# 3 "msg_055.c"

// Test for message: integral constant expression expected [55]

enum color {
	WHITE = 1.0
};
