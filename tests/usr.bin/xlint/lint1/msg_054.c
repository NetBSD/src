/*	$NetBSD: msg_054.c,v 1.2 2021/01/08 21:25:03 rillig Exp $	*/
# 3 "msg_054.c"

/* Test for message: trailing ',' prohibited in enum declaration [54] */

/* lint1-flags: -tw */

enum color {
	RED,
	GREEN,
	BLUE,
};
