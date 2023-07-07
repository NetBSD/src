/*	$NetBSD: msg_132_lp64.c,v 1.2 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_132_lp64.c"

// Test for message: conversion from '%s' to '%s' may lose accuracy [132]

/* lint1-extra-flags: -a -X 351 */
/* lint1-only-if: lp64 */

unsigned int
convert_pointer_to_smaller_integer(void *ptr)
{
	/* expect+1: warning: conversion from 'unsigned long' to 'unsigned int' may lose accuracy [132] */
	return (unsigned long)(ptr) >> 12;
}
