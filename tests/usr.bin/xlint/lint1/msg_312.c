/*	$NetBSD: msg_312.c,v 1.5 2023/07/09 11:01:27 rillig Exp $	*/
# 3 "msg_312.c"

/* Test for message: %s does not support '//' comments [312] */

/* lint1-flags: -tw */

/* expect+1: warning: traditional C does not support '//' comments [312] */
// C99 comment
