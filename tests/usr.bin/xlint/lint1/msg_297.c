/*	$NetBSD: msg_297.c,v 1.4 2023/03/28 14:44:35 rillig Exp $	*/
# 3 "msg_297.c"

// Test for message: conversion to '%s' may sign-extend incorrectly, arg #%d [297]

/* lint1-extra-flags: -P -a -p -X 351 */

void take_unsigned_long_long(unsigned long long);
void take_long_long(long long);

void
caller(signed int si, unsigned int ui)
{

	/* expect+1: warning: conversion to 'unsigned long long' may sign-extend incorrectly, arg #1 [297] */
	take_unsigned_long_long(si);

	take_unsigned_long_long(ui);

	take_long_long(si);

	/* expect+1: warning: conversion to 'long long' may sign-extend incorrectly, arg #1 [297] */
	take_long_long(ui);
}
