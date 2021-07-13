/*	$NetBSD: msg_142.c,v 1.5 2021/07/13 19:11:35 rillig Exp $	*/
# 3 "msg_142.c"

// Test for message: floating point overflow detected, op %s [142]

/* lint1-only-if ldbl-64 */
/*
 * For 96-bit and 128-bit floating point numbers, a different number of
 * multipliers is needed to produce an overflow.
 */

/* expect+2: warning: floating point overflow detected, op * [142] */
/* expect+1: warning: floating point overflow detected, op * [142] */
double dbl = 1e100 * 1e100 * 1e100 * 1e100 * 1e100;
