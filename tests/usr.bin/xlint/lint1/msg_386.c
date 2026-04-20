/*	$NetBSD: msg_386.c,v 1.4 2026/04/20 22:17:08 rillig Exp $	*/
# 3 "msg_386.c"

// Test for message: conversion '%.*s' from '%.*s' does not mix with '%c' [386]

/*
 * In the snprintb format string, the conversions 'f' and '=' mix well, and so
 * do 'F' and ':'.
 *
 * The other combinations, that is 'f' with ':', as well as 'F' with '=', need
 * more careful construction, to avoid tokens being merged together in the
 * output, and to avoid several separators next to each other.
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
	    /* expect+2: warning: conversion ':' from ':\001""no-mix\0' does not mix with 'f' [386] */
	    /* expect+1: warning: conversion '=' from '=\000""no-mix\0' does not mix with 'F' [386] */
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
