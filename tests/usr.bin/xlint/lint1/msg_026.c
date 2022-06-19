/*	$NetBSD: msg_026.c,v 1.4 2022/06/19 11:50:42 rillig Exp $	*/
# 3 "msg_026.c"

// Test for message: cannot initialize extern declaration '%s' [26]

/* expect+1: warning: cannot initialize extern declaration 'used' [26] */
extern int used = 1;
int defined = 1;
