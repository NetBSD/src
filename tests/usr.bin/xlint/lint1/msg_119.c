/*	$NetBSD: msg_119.c,v 1.4 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_119.c"

// Test for message: conversion of '%s' to '%s' is out of range [119]

/* lint1-extra-flags: -X 351 */

float
too_big(void)
{
	/* expect+1: warning: conversion of 'double' to 'float' is out of range [119] */
	return 1.0e100;
}
