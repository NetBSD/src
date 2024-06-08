/*	$NetBSD: msg_222.c,v 1.6 2024/06/08 06:37:06 rillig Exp $	*/
# 3 "msg_222.c"

// Test for message: conversion of negative constant %lld to unsigned type '%s' [222]

/* lint1-extra-flags: -X 351 */

/* expect+1: warning: initialization of unsigned type 'unsigned int' with negative constant -1 [221] */
unsigned int global = -1;

void take_unsigned_int(unsigned int);

void
function(void)
{
	/* expect+1: warning: initialization of unsigned type 'unsigned int' with negative constant -1 [221] */
	unsigned int local = -1;

	/* expect+1: warning: conversion of negative constant -1 to unsigned type 'unsigned int', arg #1 [296] */
	take_unsigned_int(-1);

	if (local & -1)
		return;

	/* expect+1: warning: operator '<' compares 'unsigned int' with 'negative constant' [162] */
	if (local < -1)
		return;

	local &= -1;

	/* expect+1: warning: conversion of negative constant -1 to unsigned type 'unsigned int' [222] */
	local += -1;
}
