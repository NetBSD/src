/*	$NetBSD: msg_265.c,v 1.2 2021/01/03 20:20:01 rillig Exp $	*/
# 3 "msg_265.c"

/* Test for message: %s C does not support 'long long' [265] */

/* lint1-flags: -w */

long long unsupported_variable;

/*LONGLONG*/
long long suppressed_variable;

long long another_unsupported_variable;
