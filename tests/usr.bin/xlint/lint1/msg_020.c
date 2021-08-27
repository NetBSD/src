/*	$NetBSD: msg_020.c,v 1.3 2021/08/27 20:16:50 rillig Exp $	*/
# 3 "msg_020.c"

// Test for message: negative array dimension (%d) [20]

/* expect+1: error: negative array dimension (-3) [20] */
int array[-3];
