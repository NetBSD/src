/*	$NetBSD: msg_178.c,v 1.2 2021/01/24 16:12:45 rillig Exp $	*/
# 3 "msg_178.c"

// Test for message: initializer does not fit [178]

char fits = 123;

char does_not_fit = 0x12345678;		/* expect: 178 */
