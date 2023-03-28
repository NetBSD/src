/*	$NetBSD: msg_178.c,v 1.4 2023/03/28 14:44:35 rillig Exp $	*/
# 3 "msg_178.c"

// Test for message: initializer does not fit [178]

/* lint1-extra-flags: -X 351 */

char fits = 123;

/* expect+1: warning: initializer does not fit [178] */
char does_not_fit = 0x12345678;
