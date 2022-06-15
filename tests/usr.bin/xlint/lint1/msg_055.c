/*	$NetBSD: msg_055.c,v 1.4 2022/06/15 20:18:31 rillig Exp $	*/
# 3 "msg_055.c"

// Test for message: integral constant expression expected [55]

enum color {
	WHITE = 1.0
};
/* expect-1: error: integral constant expression expected [55] */
