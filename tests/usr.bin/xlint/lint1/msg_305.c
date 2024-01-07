/*	$NetBSD: msg_305.c,v 1.6 2024/01/07 21:19:42 rillig Exp $	*/
# 3 "msg_305.c"

/* Test for message: conversion of %s to %s requires a cast, op %s [305] */

/* lint1-flags: -sw -X 351 */

void *void_pointer;
void (*void_function)(void);
int (*int_function)(int);

void
example(int cond)
{
	/* expect+1: warning: conversion of function pointer to 'void *' requires a cast, op = [305] */
	void_pointer = void_function;

	/* expect+1: warning: conversion of 'void *' to function pointer requires a cast, op = [305] */
	void_function = void_pointer;

	/* expect+1: warning: conversion of function pointer to 'void *' requires a cast, op = [305] */
	void_pointer = cond ? void_function : int_function;
	/* expect+1: warning: conversion of function pointer to 'void *' requires a cast, op : [305] */
	void_pointer = cond ? void_pointer : int_function;
	/* expect+1: warning: conversion of function pointer to 'void *' requires a cast, op : [305] */
	void_pointer = cond ? void_function : void_pointer;
}
