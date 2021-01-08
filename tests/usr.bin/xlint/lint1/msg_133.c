/*	$NetBSD: msg_133.c,v 1.2 2021/01/08 21:25:03 rillig Exp $	*/
# 3 "msg_133.c"

// Test for message: conversion of pointer to '%s' loses bits [133]

char
to_char(void *ptr)
{
	return (char)ptr;
}
