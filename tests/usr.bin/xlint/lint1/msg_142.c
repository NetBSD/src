/*	$NetBSD: msg_142.c,v 1.9 2023/04/10 23:52:49 rillig Exp $	*/
# 3 "msg_142.c"

// Test for message: floating point overflow on operator '%s' [142]

/* lint1-extra-flags: -X 351 */

/*
 * VAX has floating point formats with different limits than the other
 * platforms, which all implement IEEE 754.
 */
/* xlint1-skip-if: vax */

/*
 * For 96-bit and 128-bit floating point numbers, a different number of
 * multipliers is needed to produce an overflow.
 */

/* expect+2: warning: floating point overflow on operator '*' [142] */
/* expect+1: warning: floating point overflow on operator '*' [142] */
double dbl = 1e100 * 1e100 * 1e100 * 1e100 * 1e100;

/* expect+1: warning: floating point overflow on operator '+' [142] */
double _Complex complex_sum = 1e308 + 1e308i;
