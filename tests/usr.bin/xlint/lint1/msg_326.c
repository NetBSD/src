/*	$NetBSD: msg_326.c,v 1.3 2022/06/17 06:59:16 rillig Exp $	*/
# 3 "msg_326.c"

// Test for message: %s attribute ignored for %s [326]

/* expect+1: warning: packed attribute ignored for int [326] */
int variable __packed;
