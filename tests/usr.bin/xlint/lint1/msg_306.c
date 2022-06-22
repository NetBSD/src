/*	$NetBSD: msg_306.c,v 1.4 2022/06/22 19:23:18 rillig Exp $	*/
# 3 "msg_306.c"

// Test for message: constant truncated by conversion, op '%s' [306]

unsigned char
to_u8(void)
{
	/* expect+1: warning: conversion of 'int' to 'unsigned char' is out of range [119] */
	return 12345;
}

unsigned char
and_u8(unsigned char a)
{
	/* XXX: unused bits in constant */
	return a & 0x1234;
}

unsigned char
or_u8(unsigned char a)
{
	/* expect+1: warning: constant truncated by conversion, op '|=' [306] */
	a |= 0x1234;

	/* XXX: Lint doesn't care about the expanded form of the same code. */
	a = a | 0x1234;

	return a;
}
