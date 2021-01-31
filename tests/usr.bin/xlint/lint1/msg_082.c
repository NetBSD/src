/*	$NetBSD: msg_082.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_082.c"

/* Test for message: \x undefined in traditional C [82] */

/* lint1-flags: -Stw */

char str[] = "A he\x78 escape";		/* expect: 82 */
