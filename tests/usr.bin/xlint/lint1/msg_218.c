/*	$NetBSD: msg_218.c,v 1.8 2023/07/08 11:03:00 rillig Exp $	*/
# 3 "msg_218.c"

/* Test for message: ANSI C treats constant as unsigned, op '%s' [218] */

/* lint1-only-if: ilp32 */
/* lint1-flags: -w -X 351 */

int cond;
signed int s32;
unsigned int u32;
/* expect+1: warning: C90 does not support 'long long' [265] */
signed long long s64;
/* expect+1: warning: C90 does not support 'long long' [265] */
unsigned long long u64;

void sink_int(int);

/* All platforms supported by lint have 32-bit int in two's complement. */
void
test_signed_int(void)
{
	/* expect+2: warning: ANSI C treats constant as unsigned, op '-' [218] */
	/* expect+1: warning: conversion of 'unsigned long' to 'int' is out of range, arg #1 [295] */
	sink_int(-2147483648);
}

/*
 * In traditional C, integer constants with an 'L' suffix that didn't fit
 * into 'long' were promoted to the next larger integer type, if that existed
 * at all, as the suffix 'LL' was introduced by C90.
 *
 * Starting with C90, integer constants with an 'L' suffix that didn't fit
 * into 'long' were promoted to 'unsigned long' first, before trying 'long
 * long'.
 *
 * In C99 mode, this distinction is no longer necessary since it is far
 * enough from traditional C.
 */
void
compare_large_constant(void)
{
	/* expect+1: warning: ANSI C treats constant as unsigned, op '<' [218] */
	cond = s32 < 3000000000L;
	/* expect+1: warning: ANSI C treats constant as unsigned, op '<' [218] */
	cond = 3000000000L < s32;
	/* expect+1: warning: ANSI C treats constant as unsigned, op '<' [218] */
	cond = u32 < 3000000000L;
	/* expect+1: warning: ANSI C treats constant as unsigned, op '<' [218] */
	cond = 3000000000L < u32;
	/* expect+1: warning: ANSI C treats constant as unsigned, op '<' [218] */
	cond = s64 < 3000000000L;
	/* expect+1: warning: ANSI C treats constant as unsigned, op '<' [218] */
	cond = 3000000000L < s64;
	/* expect+1: warning: ANSI C treats constant as unsigned, op '<' [218] */
	cond = u64 < 3000000000L;
	/* expect+1: warning: ANSI C treats constant as unsigned, op '<' [218] */
	cond = 3000000000L < u64;
}
