/*	$NetBSD: msg_075.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_075.c"

// Test for message: overflow in hex escape [75]

char str[] = "\x12345678123456781234567812345678";	/* expect: 75 */
