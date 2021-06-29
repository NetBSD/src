/*	$NetBSD: msg_082.c,v 1.4 2021/06/29 07:17:43 rillig Exp $	*/
# 3 "msg_082.c"

/* Test for message: \x undefined in traditional C [82] */

/* lint1-flags: -Stw */

char str[] = "A he\x78 escape";		/* expect: [82] */
