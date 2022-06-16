/*	$NetBSD: msg_296.c,v 1.3 2022/06/16 21:24:41 rillig Exp $	*/
# 3 "msg_296.c"

// Test for message: conversion of negative constant to unsigned type, arg #%d [296]

void take_unsigned_int(unsigned int);

void
example(void)
{
	/* expect+1: warning: conversion of negative constant to unsigned type, arg #1 [296] */
	take_unsigned_int(-3);
}
