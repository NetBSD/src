/*	$NetBSD: msg_259.c,v 1.3 2021/01/31 13:56:14 rillig Exp $	*/
# 3 "msg_259.c"

// Test for message: conversion from '%s' to '%s' due to prototype, arg #%d [259]

/* lint1-extra-flags: -h */

void farg_char(char);
void farg_int(int);
void farg_long(long);

void
example(char c, int i, long l)
{
	farg_char(c);
	farg_int(c);
	farg_long(c);
	farg_char(i);		/* XXX: why no warning? */
	farg_int(i);
	farg_long(i);
	farg_char(l);		/* XXX: why no warning? */
	farg_int(l);		/* expect: 259 */
	farg_long(l);
}
