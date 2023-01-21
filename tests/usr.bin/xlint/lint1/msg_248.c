/*	$NetBSD: msg_248.c,v 1.4 2023/01/21 17:48:29 rillig Exp $	*/
# 3 "msg_248.c"

// Test for message: floating-point constant out of range [248]

float fits_flt = 1e37f;
/* expect+1: warning: floating-point constant out of range [248] */
float too_large_flt = 1e40f;

double fits_dbl = 1e300;
/* expect+1: warning: floating-point constant out of range [248] */
double too_large_dbl = 1e310;
