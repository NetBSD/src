/*	$NetBSD: msg_164.c,v 1.6 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_164.c"

// Test for message: assignment of negative constant to unsigned type [164]

/* lint1-extra-flags: -X 351 */

void
example(void)
{
	/* expect+1: warning: initialization of unsigned with negative constant [221] */
	unsigned char uch = -3;

	/* expect+1: warning: assignment of negative constant to unsigned type [164] */
	uch = -5;
	/* expect+1: warning: conversion of negative constant to unsigned type [222] */
	uch += -7;
	/* expect+1: warning: conversion of negative constant to unsigned type [222] */
	uch *= -1;
}
