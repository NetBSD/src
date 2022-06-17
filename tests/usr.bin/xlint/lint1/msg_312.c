/*	$NetBSD: msg_312.c,v 1.4 2022/06/17 18:54:53 rillig Exp $	*/
# 3 "msg_312.c"

/* Test for message: %s does not support // comments [312] */

/* lint1-flags: -tw */

/* expect+1: warning: traditional C does not support // comments [312] */
// C99 comment
