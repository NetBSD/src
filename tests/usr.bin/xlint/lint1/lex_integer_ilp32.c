/*	$NetBSD: lex_integer_ilp32.c,v 1.4 2022/06/17 18:54:53 rillig Exp $	*/
# 3 "lex_integer_ilp32.c"

/*
 * Tests for lexical analysis of integer constants.
 *
 * C99 6.4.4.1 "Integer constants"
 */

/* lint1-only-if: ilp32 */

void sinki(int);
void sinku(unsigned int);

/* All platforms supported by lint have 32-bit int in two's complement. */
void
test_signed_int(void)
{
	sinki(0);

	sinki(-1);

	sinki(2147483647);

	/* expect+1: warning: conversion of 'unsigned long' to 'int' is out of range, arg #1 [295] */
	sinki(2147483648);

	sinki(-2147483647);

	/* expect+2: warning: ANSI C treats constant as unsigned, op - [218] */
	/* expect+1: warning: conversion of 'unsigned long' to 'int' is out of range, arg #1 [295] */
	sinki(-2147483648);
}

void
test_unsigned_int(void)
{
	sinku(0);

	sinku(4294967295U);

	/* expect+1: warning: integer constant out of range [252] */
	sinku(4294967296U);
}
