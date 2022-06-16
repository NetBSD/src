/*	$NetBSD: msg_119.c,v 1.3 2022/06/16 16:58:36 rillig Exp $	*/
# 3 "msg_119.c"

// Test for message: conversion of '%s' to '%s' is out of range [119]

float
too_big(void)
{
	/* expect+1: warning: conversion of 'double' to 'float' is out of range [119] */
	return 1.0e100;
}
