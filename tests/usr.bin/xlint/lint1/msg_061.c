/*	$NetBSD: msg_061.c,v 1.5 2022/06/20 21:13:36 rillig Exp $	*/
# 3 "msg_061.c"

// Test for message: void parameter '%s' cannot have name [61]

/* expect+1: error: void parameter 'arg' cannot have name [61] */
void example(void arg);
