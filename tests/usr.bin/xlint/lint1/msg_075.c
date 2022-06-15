/*	$NetBSD: msg_075.c,v 1.4 2022/06/15 20:18:31 rillig Exp $	*/
# 3 "msg_075.c"

// Test for message: overflow in hex escape [75]

/* expect+1: warning: overflow in hex escape [75] */
char str[] = "\x12345678123456781234567812345678";
