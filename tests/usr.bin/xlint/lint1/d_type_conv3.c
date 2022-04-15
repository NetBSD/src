/*	$NetBSD: d_type_conv3.c,v 1.6 2022/04/15 21:50:07 rillig Exp $	*/
# 3 "d_type_conv3.c"

/* Flag information-losing type conversion in argument lists */

/*
 * Before tree.c 1.427 from 2022-04-15, lint warned about conversions due to
 * prototype even in C99 mode, which is far away from traditional C to make
 * non-prototype functions an issue.
 */
/* lint1-flags: -g -h -w */

int f(unsigned int);

void
should_fail(void)
{

	/* expect+2: warning: argument #1 is converted from 'long long' to 'unsigned int' due to prototype [259] */
	/* expect+1: warning: conversion of 'long long' to 'unsigned int' is out of range, arg #1 [295] */
	f(0x7fffffffffffffffLL);
}
