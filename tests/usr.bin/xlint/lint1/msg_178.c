/*	$NetBSD: msg_178.c,v 1.3 2022/06/16 16:58:36 rillig Exp $	*/
# 3 "msg_178.c"

// Test for message: initializer does not fit [178]

char fits = 123;

/* expect+1: warning: initializer does not fit [178] */
char does_not_fit = 0x12345678;
