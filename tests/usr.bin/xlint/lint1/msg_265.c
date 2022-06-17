/*	$NetBSD: msg_265.c,v 1.5 2022/06/17 18:54:53 rillig Exp $	*/
# 3 "msg_265.c"

/* Test for message: %s does not support 'long long' [265] */

/* lint1-flags: -w */

/* expect+1: warning: C90 does not support 'long long' [265] */
long long unsupported_variable;

/*LONGLONG*/
long long suppressed_variable,
    second_suppressed_variable;

/* expect+1: warning: C90 does not support 'long long' [265] */
long long another_unsupported_variable;
