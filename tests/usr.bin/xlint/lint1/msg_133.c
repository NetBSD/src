/*	$NetBSD: msg_133.c,v 1.5 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_133.c"

// Test for message: conversion of pointer to '%s' loses bits [133]

/* lint1-extra-flags: -X 351 */

char
to_char(void *ptr)
{
	/* expect+1: warning: conversion of pointer to 'char' loses bits [133] */
	return (char)ptr;
}
