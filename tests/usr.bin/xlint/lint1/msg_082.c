/*	$NetBSD: msg_082.c,v 1.5 2022/06/15 20:18:31 rillig Exp $	*/
# 3 "msg_082.c"

/* Test for message: \x undefined in traditional C [82] */

/* lint1-flags: -Stw */

/* expect+1: warning: \x undefined in traditional C [82] */
char str[] = "A he\x78 escape";
