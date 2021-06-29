/*	$NetBSD: lex_integer.c,v 1.4 2021/06/29 13:58:13 rillig Exp $	*/
# 3 "lex_integer.c"

/*
 * Tests for lexical analysis of integer constants.
 *
 * C99 6.4.4.1 "Integer constants"
 */

/* lint1-only-if lp64 */

void sinki(int);
void sinku(unsigned int);

/* All platforms supported by lint have 32-bit int in two's complement. */
void
test_signed_int(void)
{
	sinki(0);

	sinki(-1);

	sinki(2147483647);

	/* expect+2: converted from 'long' to 'int' due to prototype */
	/* expect+1: conversion of 'long' to 'int' is out of range */
	sinki(2147483648);

	sinki(-2147483647);

	/* expect+1: converted from 'long' to 'int' due to prototype */
	sinki(-2147483648);
}

void
test_unsigned_int(void)
{
	sinku(0);

	sinku(4294967295U);

	/* expect+2: from 'unsigned long' to 'unsigned int' due to prototype */
	/* expect+1: conversion of 'unsigned long' to 'unsigned int' is out of range */
	sinku(4294967296U);
}
