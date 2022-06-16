/*	$NetBSD: msg_276.c,v 1.3 2022/06/16 21:24:41 rillig Exp $	*/
# 3 "msg_276.c"

// Test for message: __%s__ is illegal for type %s [276]

/* expect+1: error: __real__ is illegal for type double [276] */
int real_int = __real__ 0.0;
/* expect+1: error: __imag__ is illegal for type double [276] */
int imag_int = __imag__ 0.0;
