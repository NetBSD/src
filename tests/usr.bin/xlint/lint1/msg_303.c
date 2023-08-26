/*	$NetBSD: msg_303.c,v 1.5 2023/08/26 10:43:53 rillig Exp $	*/
# 3 "msg_303.c"

/* Test for message: conversion of %s to %s requires a cast [303] */

/* lint1-flags: -sw -X 351 */

void take_void_pointer(void *);

void *
to_void_pointer(void)
{
	/* expect+1: warning: conversion of function pointer to 'void *' requires a cast [303] */
	return to_void_pointer;
}

void (*to_function_pointer(void *arg))(void)
{
	/* expect+1: warning: conversion of 'void *' to function pointer requires a cast [303] */
	return arg;
}
