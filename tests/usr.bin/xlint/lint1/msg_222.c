/*	$NetBSD: msg_222.c,v 1.7 2024/12/15 05:56:18 rillig Exp $	*/
# 3 "msg_222.c"

// Test for message: conversion of negative constant %lld to unsigned type '%s' [222]

/* lint1-extra-flags: -X 351 */

/* expect+1: warning: initialization of unsigned type 'unsigned int' with negative constant -1 [221] */
unsigned int global = -1;

void take_unsigned_int(unsigned int);

void
function(signed char *scp, unsigned char *ucp)
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

	*scp += 'A' - 'a';
	*scp -= 'A' - 'a';

	// XXX: It's perfectly fine to effectively subtract a constant from
	// XXX: an unsigned type.
	/* expect+1: warning: conversion of negative constant -32 to unsigned type 'unsigned char' [222] */
	*ucp += 'A' - 'a';
	/* expect+1: warning: conversion of negative constant -32 to unsigned type 'unsigned char' [222] */
	*ucp -= 'A' - 'a';
}
