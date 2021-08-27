/*	$NetBSD: msg_252.c,v 1.3 2021/08/27 20:49:25 rillig Exp $	*/
# 3 "msg_252.c"

// Test for message: integer constant out of range [252]

/* expect+1: warning: integer constant out of range [252] */
int constant = 1111111111111111111111111111111111111111111111111111;
