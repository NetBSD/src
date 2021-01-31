/*	$NetBSD: msg_265.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_265.c"

/* Test for message: %s C does not support 'long long' [265] */

/* lint1-flags: -w */

long long unsupported_variable;			/* expect: 265 */

/*LONGLONG*/
long long suppressed_variable;

long long another_unsupported_variable;		/* expect: 265 */
