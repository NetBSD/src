/*	$NetBSD: msg_276.c,v 1.6 2025/04/12 15:49:50 rillig Exp $	*/
# 3 "msg_276.c"

// Test for message: '__%s__' is invalid for type '%s' [276]

/* lint1-extra-flags: -X 351 */

/* expect+1: error: '__real__' is invalid for type 'double' [276] */
int real_int = __real__ 0.0;
/* expect+1: error: '__imag__' is invalid for type 'double' [276] */
int imag_int = __imag__ 0.0;
