/*	$NetBSD: msg_061.c,v 1.4 2022/06/15 20:18:31 rillig Exp $	*/
# 3 "msg_061.c"

// Test for message: void parameter cannot have name: %s [61]

/* expect+1: error: void parameter cannot have name: arg [61] */
void example(void arg);
