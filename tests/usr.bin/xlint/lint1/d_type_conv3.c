/*	$NetBSD: d_type_conv3.c,v 1.5 2022/01/15 14:22:03 rillig Exp $	*/
# 3 "d_type_conv3.c"

/* Flag information-losing type conversion in argument lists */

int f(unsigned int);

void
should_fail()
{

	/* expect+2: warning: argument #1 is converted from 'long long' to 'unsigned int' due to prototype [259] */
	/* expect+1: warning: conversion of 'long long' to 'unsigned int' is out of range, arg #1 [295] */
	f(0x7fffffffffffffffLL);
}
