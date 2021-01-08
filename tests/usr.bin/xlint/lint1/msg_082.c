/*	$NetBSD: msg_082.c,v 1.2 2021/01/08 21:25:03 rillig Exp $	*/
# 3 "msg_082.c"

/* Test for message: \x undefined in traditional C [82] */

/* lint1-flags: -Stw */

char str[] = "A he\x78 escape";
