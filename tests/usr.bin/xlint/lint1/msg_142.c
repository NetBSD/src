/*	$NetBSD: msg_142.c,v 1.8 2023/03/28 14:44:34 rillig Exp $	*/
# 3 "msg_142.c"

// Test for message: floating point overflow on operator '%s' [142]

/* lint1-only-if: ldbl-64 */
/* lint1-extra-flags: -X 351 */

/*
 * For 96-bit and 128-bit floating point numbers, a different number of
 * multipliers is needed to produce an overflow.
 */

/* expect+2: warning: floating point overflow on operator '*' [142] */
/* expect+1: warning: floating point overflow on operator '*' [142] */
double dbl = 1e100 * 1e100 * 1e100 * 1e100 * 1e100;
