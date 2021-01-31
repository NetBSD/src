/*	$NetBSD: msg_061.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_061.c"

// Test for message: void parameter cannot have name: %s [61]

void example(void arg);		/* expect: 61 */
