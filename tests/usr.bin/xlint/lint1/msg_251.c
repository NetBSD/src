/*	$NetBSD: msg_251.c,v 1.3 2021/06/29 07:17:43 rillig Exp $	*/
# 3 "msg_251.c"

// Test for message: malformed integer constant [251]

/* expect+1: malformed integer constant */
int lll = 123LLL;

/* expect+1: malformed integer constant */
unsigned int uu = 123UU;
