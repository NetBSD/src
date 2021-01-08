/*	$NetBSD: msg_144.c,v 1.2 2021/01/08 21:25:03 rillig Exp $	*/
# 3 "msg_144.c"

// Test for message: cannot take size/alignment of function [144]

unsigned long
example(void)
{
	return sizeof example;
}
