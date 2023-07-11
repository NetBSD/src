/*	$NetBSD: msg_054.c,v 1.5 2023/07/11 20:54:23 rillig Exp $	*/
# 3 "msg_054.c"

/* Test for message: trailing ',' in enum declaration requires C99 or later [54] */

/* lint1-flags: -sw */

enum color {
	RED,
	GREEN,
	BLUE,
};
/* expect-1: error: trailing ',' in enum declaration requires C99 or later [54] */
