/*	$NetBSD: msg_076.c,v 1.3 2021/06/29 07:17:43 rillig Exp $	*/
# 3 "msg_076.c"

// Test for message: character escape does not fit in character [76]

char ch = '\777';		/* expect: [76] */
