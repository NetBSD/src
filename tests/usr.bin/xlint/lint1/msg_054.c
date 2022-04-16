/*	$NetBSD: msg_054.c,v 1.4 2022/04/16 09:22:25 rillig Exp $	*/
# 3 "msg_054.c"

/* Test for message: trailing ',' prohibited in enum declaration [54] */

/* lint1-flags: -sw */

enum color {
	RED,
	GREEN,
	BLUE,
};
/* expect-1: error: trailing ',' prohibited in enum declaration [54] */
