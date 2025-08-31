/*	$NetBSD: msg_386.c,v 1.1 2025/08/31 20:43:27 rillig Exp $	*/
# 3 "msg_386.c"

// Test for message: conversion '%.*s' does not mix with '%c' [386]

/*
 * In the snprintb format string, the conversions 'f' and '=' mix well, and so
 * do 'F' and ':'.  But 'f' doesn't mix with ':', and neither does 'F' mix with
 * '='.
 */

/* lint1-extra-flags: -X 351 */

typedef typeof(sizeof 0) size_t;

void snprintb(char *, size_t, const char *, unsigned long long);

void
test_snprintb(void)
{
	char buf[50];

	snprintb(buf, sizeof buf,
	    "\177\020"
	    "f\000\020" "field\0"
	    "" "=\000" "mix\0"
	    "" ":\001" "no-mix\0"
	    "F\020\020" "field\0"
	    "" "=\000" "no-mix\0"
	    "" ":\000" "mix\0",
	    /* expect+2: warning: conversion ':' does not mix with 'f' [386] */
	    /* expect+1: warning: conversion '=' does not mix with 'F' [386] */
	    0xffffffff);
}
