/*	$NetBSD: msg_298.c,v 1.7.2.1 2025/08/02 05:58:18 perseant Exp $	*/
# 3 "msg_298.c"

// Test for message: conversion from '%s' to '%s' may lose accuracy, arg #%d [298]

/* lint1-extra-flags: -a -X 351 */

void take_uchar(unsigned char);
void take_schar(signed char);
void take_uint(unsigned int);
void take_int(int);

void
convert_bit_and(long l)
{
	/* expect+1: warning: conversion from 'long' to 'unsigned char' may lose accuracy, arg #1 [298] */
	take_uchar(l);
	take_uchar(l & 0xFF);
	/* expect+1: warning: conversion from 'long' to 'unsigned char' may lose accuracy, arg #1 [298] */
	take_uchar(l & 0x100);
	/* expect+1: warning: conversion from 'long' to 'signed char' may lose accuracy, arg #1 [298] */
	take_schar(l & 0xFF);
	take_schar(l & 0x7F);
}

void
convert_mult(long long ll)
{
	/* expect+1: warning: conversion from 'long long' to 'int' may lose accuracy, arg #1 [298] */
	take_int(ll * 2);
}

// For lossy floating-to-integer conversions, see messages 380 and 381.
