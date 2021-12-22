/*	$NetBSD: msg_312.c,v 1.3 2021/12/22 14:25:35 rillig Exp $	*/
# 3 "msg_312.c"

/* Test for message: %s does not support // comments [312] */

/* lint1-flags: -tw */

/* expect+1: traditional C does not support // comments [312] */
// C99 comment
