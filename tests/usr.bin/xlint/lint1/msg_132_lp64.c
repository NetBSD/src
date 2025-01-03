/*	$NetBSD: msg_132_lp64.c,v 1.3 2025/01/03 01:27:35 rillig Exp $	*/
# 3 "msg_132_lp64.c"

// Test for message: conversion from '%s' to '%s' may lose accuracy [132]

/* lint1-extra-flags: -a -X 351 */
/* lint1-only-if: lp64 */

typedef unsigned int u32_t;

u32_t u32;
const char *ptr;

unsigned int
convert_pointer_to_smaller_integer(void)
{
	/* expect+1: warning: conversion from 'unsigned long' to 'unsigned int' may lose accuracy [132] */
	return (unsigned long)(ptr) >> 12;
}

void
test_ic_minus(void)
{
	/* expect+1: warning: conversion from 'long' to 'unsigned int' may lose accuracy [132] */
	u32 = ptr + 3 - ptr;
}
