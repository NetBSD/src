/*	$NetBSD: d_constant_conv1.c,v 1.6 2024/06/08 06:37:06 rillig Exp $	*/
# 3 "d_constant_conv1.c"

/* Flag information-losing constant conversion in argument lists */

/* lint1-extra-flags: -X 351 */

int f(unsigned int);

void
should_fail()
{
	/* expect+1: warning: conversion of negative constant -1 to unsigned type 'unsigned int', arg #1 [296] */
	f(-1);
}
