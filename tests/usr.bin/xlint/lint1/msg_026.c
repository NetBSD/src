/*	$NetBSD: msg_026.c,v 1.3 2022/04/05 23:09:19 rillig Exp $	*/
# 3 "msg_026.c"

// Test for message: cannot initialize extern declaration: %s [26]

/* expect+1: warning: cannot initialize extern declaration: used [26] */
extern int used = 1;
int defined = 1;
