/*	$NetBSD: msg_164.c,v 1.7 2024/06/08 06:37:06 rillig Exp $	*/
# 3 "msg_164.c"

// Test for message: assignment of negative constant %lld to unsigned type '%s' [164]

/* lint1-extra-flags: -X 351 */

void
example(void)
{
	/* expect+1: warning: initialization of unsigned type 'unsigned char' with negative constant -3 [221] */
	unsigned char uch = -3;

	/* expect+1: warning: assignment of negative constant -5 to unsigned type 'unsigned char' [164] */
	uch = -5;
	/* expect+1: warning: conversion of negative constant -7 to unsigned type 'unsigned char' [222] */
	uch += -7;
	/* expect+1: warning: conversion of negative constant -1 to unsigned type 'unsigned char' [222] */
	uch *= -1;
}
