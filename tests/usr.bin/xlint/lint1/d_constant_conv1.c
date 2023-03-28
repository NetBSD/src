/*	$NetBSD: d_constant_conv1.c,v 1.5 2023/03/28 14:44:34 rillig Exp $	*/
# 3 "d_constant_conv1.c"

/* Flag information-losing constant conversion in argument lists */

/* lint1-extra-flags: -X 351 */

int f(unsigned int);

void
should_fail()
{
	/* expect+1: warning: conversion of negative constant to unsigned type, arg #1 [296] */
	f(-1);
}
