/*	$NetBSD: msg_133.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_133.c"

// Test for message: conversion of pointer to '%s' loses bits [133]

char
to_char(void *ptr)
{
	return (char)ptr;	/* expect: 133 */
}
