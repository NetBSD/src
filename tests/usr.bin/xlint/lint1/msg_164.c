/*	$NetBSD: msg_164.c,v 1.5 2022/06/16 16:58:36 rillig Exp $	*/
# 3 "msg_164.c"

// Test for message: assignment of negative constant to unsigned type [164]

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
