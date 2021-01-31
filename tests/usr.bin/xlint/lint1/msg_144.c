/*	$NetBSD: msg_144.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_144.c"

// Test for message: cannot take size/alignment of function [144]

unsigned long
example(void)
{
	return sizeof example;	/* expect: 144 */
}
