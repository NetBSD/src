/*	$NetBSD: msg_076.c,v 1.4 2022/06/15 20:18:31 rillig Exp $	*/
# 3 "msg_076.c"

// Test for message: character escape does not fit in character [76]

/* expect+1: warning: character escape does not fit in character [76] */
char ch = '\777';
