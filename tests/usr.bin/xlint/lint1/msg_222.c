/*	$NetBSD: msg_222.c,v 1.8 2024/12/15 07:43:53 rillig Exp $	*/
# 3 "msg_222.c"

// Test for message: conversion of negative constant %lld to unsigned type '%s' [222]
//
// See also:
//	msg_162.c: comparison of unsigned type with negative constant
//	msg_164.c: assignment of negative constant to unsigned type
//	msg_221.c: initialization of unsigned type with negative constant
//	msg_296.c: conversion of negative constant to unsigned type in call

/* lint1-extra-flags: -X 351 */

unsigned int u32;
signed char sc;
unsigned char uc;
_Bool b;


void
convert_negative_constant(void)
{
	u32 = !-8;
	u32 = ~-8;
	/* expect+1: warning: assignment of negative constant -8 to unsigned type 'unsigned int' [164] */
	u32 = +-8;
	u32 = - -8;

	/* expect+1: warning: conversion of negative constant -8 to unsigned type 'unsigned int' [222] */
	u32 = u32 * -8;
	/* expect+1: warning: conversion of negative constant -8 to unsigned type 'unsigned int' [222] */
	u32 = -8 * u32;
	/* expect+1: warning: conversion of negative constant -8 to unsigned type 'unsigned int' [222] */
	u32 = u32 / -8;
	/* expect+1: warning: conversion of negative constant -8 to unsigned type 'unsigned int' [222] */
	u32 = -8 / u32;
	/* expect+1: warning: conversion of negative constant -8 to unsigned type 'unsigned int' [222] */
	u32 = u32 % -8;
	/* expect+1: warning: conversion of negative constant -8 to unsigned type 'unsigned int' [222] */
	u32 = -8 / u32;
	/* expect+1: warning: conversion of negative constant -8 to unsigned type 'unsigned int' [222] */
	u32 = u32 + -8;
	/* expect+1: warning: conversion of negative constant -8 to unsigned type 'unsigned int' [222] */
	u32 = -8 + u32;
	/* expect+1: warning: conversion of negative constant -8 to unsigned type 'unsigned int' [222] */
	u32 = u32 - -8;
	/* expect+1: warning: conversion of negative constant -8 to unsigned type 'unsigned int' [222] */
	u32 = -8 - u32;
	/* expect+1: warning: negative shift [121] */
	u32 = u32 << -8;
	u32 = -8 << u32;
	/* expect+1: warning: negative shift [121] */
	u32 = u32 >> -8;
	u32 = -8 >> u32;

	/* expect+1: warning: operator '<' compares 'unsigned int' with 'negative constant' [162] */
	b = u32 < -8;
	/* expect+1: warning: operator '<=' compares 'unsigned int' with 'negative constant' [162] */
	b = u32 <= -8;
	/* expect+1: warning: operator '>' compares 'unsigned int' with 'negative constant' [162] */
	b = u32 > -8;
	/* expect+1: warning: operator '>=' compares 'unsigned int' with 'negative constant' [162] */
	b = u32 >= -8;
	/* expect+1: warning: operator '==' compares 'unsigned int' with 'negative constant' [162] */
	b = u32 == -8;
	/* expect+1: warning: operator '!=' compares 'unsigned int' with 'negative constant' [162] */
	b = u32 != -8;

	u32 = u32 & -8;
	u32 = u32 ^ -8;
	u32 = u32 | -8;
	b = u32 && -8;
	b = u32 || -8;

	/* expect+1: warning: assignment of negative constant -8 to unsigned type 'unsigned int' [164] */
	u32 = -8;
	/* expect+1: warning: conversion of negative constant -8 to unsigned type 'unsigned int' [222] */
	u32 *= -8;
	/* expect+1: warning: conversion of negative constant -8 to unsigned type 'unsigned int' [222] */
	u32 /= -8;
	/* expect+1: warning: conversion of negative constant -8 to unsigned type 'unsigned int' [222] */
	u32 %= -8;
	/* expect+1: warning: conversion of negative constant -8 to unsigned type 'unsigned int' [222] */
	u32 += -8;
	/* expect+1: warning: conversion of negative constant -8 to unsigned type 'unsigned int' [222] */
	u32 -= -8;
	// XXX: missing 'negative shift' warning
	u32 <<= -8;
	// XXX: missing 'negative shift' warning
	u32 >>= -8;
	u32 &= -8;
	/* expect+1: warning: conversion of negative constant -8 to unsigned type 'unsigned int' [222] */
	u32 ^= -8;
	u32 |= -8;

	sc += 'A' - 'a';
	sc -= 'A' - 'a';

	// XXX: It's perfectly fine to effectively subtract a constant from
	// XXX: an unsigned type.
	/* expect+1: warning: conversion of negative constant -32 to unsigned type 'unsigned char' [222] */
	uc += 'A' - 'a';
	/* expect+1: warning: conversion of negative constant -32 to unsigned type 'unsigned char' [222] */
	uc -= 'A' - 'a';
}
