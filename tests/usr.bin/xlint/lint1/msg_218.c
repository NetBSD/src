/*	$NetBSD: msg_218.c,v 1.4 2022/06/22 19:23:18 rillig Exp $	*/
# 3 "msg_218.c"

// Test for message: ANSI C treats constant as unsigned, op '%s' [218]

/* lint1-only-if: ilp32 */

void sink_int(int);

/* All platforms supported by lint have 32-bit int in two's complement. */
void
test_signed_int(void)
{
	/* expect+2: warning: ANSI C treats constant as unsigned, op '-' [218] */
	/* expect+1: warning: conversion of 'unsigned long' to 'int' is out of range, arg #1 [295] */
	sink_int(-2147483648);
}
