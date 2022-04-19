/*	$NetBSD: msg_298.c,v 1.3 2022/04/19 19:56:29 rillig Exp $	*/
# 3 "msg_298.c"

// Test for message: conversion from '%s' to '%s' may lose accuracy, arg #%d [298]

/* lint1-extra-flags: -a */

void take_uchar(unsigned char);
void take_schar(signed char);

void
convert_bit_and(long l)
{
	/* expect+1: warning: conversion from 'long' to 'unsigned char' may lose accuracy, arg #1 [298] */
	take_uchar(l);
	/* expect+1: warning: conversion from 'long' to 'unsigned char' may lose accuracy, arg #1 [298] */
	take_uchar(l & 0xFF);
	/* expect+1: warning: conversion from 'long' to 'unsigned char' may lose accuracy, arg #1 [298] */
	take_uchar(l & 0x100);
	/* expect+1: warning: conversion from 'long' to 'signed char' may lose accuracy, arg #1 [298] */
	take_schar(l & 0xFF);
	/* expect+1: warning: conversion from 'long' to 'signed char' may lose accuracy, arg #1 [298] */
	take_schar(l & 0x7F);
}
