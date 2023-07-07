/*	$NetBSD: msg_012.c,v 1.5 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_012.c"

// Test for message: compiler takes size of function [12]
/* This message is not used. */

/* lint1-extra-flags: -X 351 */

unsigned long
example(void)
{
	/* expect+1: error: cannot take size/alignment of function type 'function(void) returning unsigned long' [144] */
	return sizeof(example);
}
