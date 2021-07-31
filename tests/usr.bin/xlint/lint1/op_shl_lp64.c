/*	$NetBSD: op_shl_lp64.c,v 1.2 2021/07/31 19:52:44 rillig Exp $	*/
# 3 "op_shl_lp64.c"

/*
 * Before decl.c 1.215 from 2021-07-31, lint wrongly treated __uint128_t and
 * __int128_t as being equivalent to a missing type specifier, thereby
 * defaulting to int.  This led to warnings like:
 *
 *	shift amount 105 is greater than bit-size 32 of 'int' [122]
 *
 * These warnings had been discovered in ecp_nistp256.c(296).
 */

/* lint1-only-if lp64 */

const __uint128_t zero105 =
    (((__uint128_t)1) << 105)
    - (((__uint128_t)1) << 41)
    - (((__uint128_t)1) << 9);

const __uint128_t shl_128_129 =
    /* expect+1: warning: shift equal to size of object [267] */
    (((__uint128_t)1) << 128)
    /* expect+1: warning: shift amount 129 is greater than bit-size 128 of '__uint128_t' [122] */
    - (((__uint128_t)1) << 129);
