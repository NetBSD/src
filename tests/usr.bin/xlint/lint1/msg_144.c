/*	$NetBSD: msg_144.c,v 1.4 2021/04/02 12:16:50 rillig Exp $	*/
# 3 "msg_144.c"

// Test for message: cannot take size/alignment of function [144]

unsigned long
example(void)
{
	return sizeof(example);	/* expect: 144 */
}
