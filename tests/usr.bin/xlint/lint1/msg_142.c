/*	$NetBSD: msg_142.c,v 1.11 2023/07/09 11:01:27 rillig Exp $	*/
# 3 "msg_142.c"

// Test for message: operator '%s' produces floating point overflow [142]

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

/* expect+2: warning: operator '*' produces floating point overflow [142] */
/* expect+1: warning: operator '*' produces floating point overflow [142] */
double dbl = 1e100 * 1e100 * 1e100 * 1e100 * 1e100;

/*
 * Ensure that an addition in the complex number space doesn't generate
 * wrong warnings. Lint doesn't model complex constants accurately.
 */
double _Complex complex_sum = 1e308 + 1e308i;
