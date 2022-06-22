/*	$NetBSD: msg_326.c,v 1.4 2022/06/22 19:23:18 rillig Exp $	*/
# 3 "msg_326.c"

// Test for message: attribute '%s' ignored for '%s' [326]

/* expect+1: warning: attribute 'packed' ignored for 'int' [326] */
int variable __packed;
