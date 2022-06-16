/*	$NetBSD: msg_133.c,v 1.4 2022/06/16 16:58:36 rillig Exp $	*/
# 3 "msg_133.c"

// Test for message: conversion of pointer to '%s' loses bits [133]

char
to_char(void *ptr)
{
	/* expect+1: warning: conversion of pointer to 'char' loses bits [133] */
	return (char)ptr;
}
