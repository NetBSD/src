/*	$NetBSD: msg_055.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_055.c"

// Test for message: integral constant expression expected [55]

enum color {
	WHITE = 1.0
};				/* expect: 55 */
