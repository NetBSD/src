/*	$NetBSD: msg_305.c,v 1.5 2023/08/26 10:43:53 rillig Exp $	*/
# 3 "msg_305.c"

/* Test for message: conversion of %s to %s requires a cast, op %s [305] */

/* lint1-flags: -sw -X 351 */

void take_void_pointer(void *);

typedef void (*function)(void);

void
caller(void **void_pointer, function *function_pointer)
{
	/* expect+1: warning: conversion of function pointer to 'void *' requires a cast, op = [305] */
	*void_pointer = *function_pointer;

	/* expect+1: warning: conversion of 'void *' to function pointer requires a cast, op = [305] */
	*function_pointer = *void_pointer;
}
