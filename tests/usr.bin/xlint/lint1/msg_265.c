/*	$NetBSD: msg_265.c,v 1.6 2023/03/28 14:44:35 rillig Exp $	*/
# 3 "msg_265.c"

/* Test for message: %s does not support 'long long' [265] */

/* lint1-flags: -w -X 351 */

/* expect+1: warning: C90 does not support 'long long' [265] */
long long unsupported_variable;

/*LONGLONG*/
long long suppressed_variable,
    second_suppressed_variable;

/* expect+1: warning: C90 does not support 'long long' [265] */
long long another_unsupported_variable;
