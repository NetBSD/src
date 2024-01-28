/*	$NetBSD: msg_218.c,v 1.10 2024/01/28 06:57:41 rillig Exp $	*/
# 3 "msg_218.c"

/* Test for message: C90 treats constant as unsigned, op '%s' [218] */

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
	/* expect+3: warning: integer constant out of range [252] */
	/* expect+2: warning: C90 treats constant as unsigned, op '-' [218] */
	/* expect+1: warning: conversion of 'unsigned long' to 'int' is out of range, arg #1 [295] */
	sink_int(-2147483648);
}

/*
 * TODO: Investigate whether the message 218 is actually correct.
 * See C1978 2.4.1 "Integer constants" and 6.6 "Arithmetic conversions".
 */
void
compare_large_constant(void)
{
	/* expect+2: warning: integer constant out of range [252] */
	/* expect+1: warning: C90 treats constant as unsigned, op '<' [218] */
	cond = s32 < 3000000000L;
	/* expect+2: warning: integer constant out of range [252] */
	/* expect+1: warning: C90 treats constant as unsigned, op '<' [218] */
	cond = 3000000000L < s32;
	/* expect+2: warning: integer constant out of range [252] */
	/* expect+1: warning: C90 treats constant as unsigned, op '<' [218] */
	cond = u32 < 3000000000L;
	/* expect+2: warning: integer constant out of range [252] */
	/* expect+1: warning: C90 treats constant as unsigned, op '<' [218] */
	cond = 3000000000L < u32;
	/* expect+2: warning: integer constant out of range [252] */
	/* expect+1: warning: C90 treats constant as unsigned, op '<' [218] */
	cond = s64 < 3000000000L;
	/* expect+2: warning: integer constant out of range [252] */
	/* expect+1: warning: C90 treats constant as unsigned, op '<' [218] */
	cond = 3000000000L < s64;
	/* expect+2: warning: integer constant out of range [252] */
	/* expect+1: warning: C90 treats constant as unsigned, op '<' [218] */
	cond = u64 < 3000000000L;
	/* expect+2: warning: integer constant out of range [252] */
	/* expect+1: warning: C90 treats constant as unsigned, op '<' [218] */
	cond = 3000000000L < u64;
}
