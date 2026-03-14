/*	$NetBSD: msg_386.c,v 1.2 2026/03/14 15:10:25 rillig Exp $	*/
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
	snprintb((void *)0, 0,
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

	// When the value description starts with a punctuation character,
	// or more broadly, not an identifier character, it can be visually
	// distinguished, so this variant is fine.
	snprintb(
	    (void*)0, 0,
	    "\177\20"
	    "f\000\020" "field\0"
	    "" ":\0" "(zero)\0",
	    0xffff);
}
