/*	$NetBSD: msg_066.c,v 1.5 2021/08/26 19:23:25 rillig Exp $	*/
# 3 "msg_066.c"

// Test for message: syntax requires ';' after last struct/union member [66]
/* This message is not used. */

/*
 * This message was removed in cgram.y 1.328 from 2021-07-15 because all
 * C standards and even K&R require a semicolon.
 */

struct number {
	int value
};
/* expect-1: syntax error '}' [249] */
/* expect+1: error: cannot recover from previous errors [224] */
