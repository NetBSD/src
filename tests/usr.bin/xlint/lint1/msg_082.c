/*	$NetBSD: msg_082.c,v 1.6 2023/03/28 14:44:34 rillig Exp $	*/
# 3 "msg_082.c"

/* Test for message: \x undefined in traditional C [82] */

/* lint1-flags: -Stw -X 351 */

/* expect+1: warning: \x undefined in traditional C [82] */
char str[] = "A he\x78 escape";
