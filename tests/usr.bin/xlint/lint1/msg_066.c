/*	$NetBSD: msg_066.c,v 1.6 2022/06/17 18:54:53 rillig Exp $	*/
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
/* expect-1: error: syntax error '}' [249] */
/* expect+1: error: cannot recover from previous errors [224] */
