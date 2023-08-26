/*	$NetBSD: msg_304.c,v 1.5 2023/08/26 10:43:53 rillig Exp $	*/
# 3 "msg_304.c"

/* Test for message: conversion of %s to %s requires a cast, arg #%d [304] */

/* lint1-flags: -sw -X 351 */

void take_void_pointer(void *);
void take_function_pointer(void (*)(void));

void
caller(void *arg)
{
	/* expect+1: warning: conversion of function pointer to 'void *' requires a cast, arg #1 [304] */
	take_void_pointer(caller);

	/* expect+1: warning: conversion of 'void *' to function pointer requires a cast, arg #1 [304] */
	take_function_pointer(arg);
}
