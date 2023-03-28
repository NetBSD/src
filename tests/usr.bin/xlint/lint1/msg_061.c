/*	$NetBSD: msg_061.c,v 1.6 2023/03/28 14:44:34 rillig Exp $	*/
# 3 "msg_061.c"

// Test for message: void parameter '%s' cannot have name [61]

/* lint1-extra-flags: -X 351 */

/* expect+1: error: void parameter 'arg' cannot have name [61] */
void example(void arg);
