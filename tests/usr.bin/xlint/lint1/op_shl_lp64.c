/*	$NetBSD: op_shl_lp64.c,v 1.1 2021/07/04 20:22:31 rillig Exp $	*/
# 3 "op_shl_lp64.c"

/*
 * Test overflow on shl of 128-bit integers, as seen in
 * ecp_nistp256.c(296).
 */

/* lint1-only-if lp64 */

const __uint128_t zero105 =
    /* FIXME: 105 is ok for __uint128_t */
    /* expect+1: warning: shift amount 105 is greater than bit-size 32 of 'int' [122] */
    (((__uint128_t)1) << 105)
    /* FIXME: 41 is ok for __uint128_t */
    /* expect+1: warning: shift amount 41 is greater than bit-size 32 of 'int' [122] */
    - (((__uint128_t)1) << 41)
    - (((__uint128_t)1) << 9);
