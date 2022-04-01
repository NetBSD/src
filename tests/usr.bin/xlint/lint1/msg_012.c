/*	$NetBSD: msg_012.c,v 1.4 2022/04/01 23:16:32 rillig Exp $	*/
# 3 "msg_012.c"

// Test for message: compiler takes size of function [12]
/* This message is not used. */

unsigned long
example(void)
{
	/* expect+1: error: cannot take size/alignment of function type 'function(void) returning unsigned long' [144] */
	return sizeof(example);
}
