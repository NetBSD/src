/*	$NetBSD: msg_054.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_054.c"

/* Test for message: trailing ',' prohibited in enum declaration [54] */

/* lint1-flags: -tw */

enum color {
	RED,
	GREEN,
	BLUE,
};				/* expect: 54 */
