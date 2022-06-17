/*	$NetBSD: msg_251.c,v 1.4 2022/06/17 18:54:53 rillig Exp $	*/
# 3 "msg_251.c"

// Test for message: malformed integer constant [251]

/* expect+1: warning: malformed integer constant [251] */
int lll = 123LLL;

/* expect+1: warning: malformed integer constant [251] */
unsigned int uu = 123UU;
