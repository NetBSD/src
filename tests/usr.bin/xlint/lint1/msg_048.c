/*	$NetBSD: msg_048.c,v 1.4 2022/04/08 21:48:19 rillig Exp $	*/
# 3 "msg_048.c"

// Test for message: overflow in enumeration values: %s [48]

/*
 * Before decl.c 1.269 from 2022-04-08, the comparison for enum constant
 * overflow was done in signed arithmetic, and since 'enumval' wrapped
 * around, its value became INT_MIN, at least on platforms where integer
 * overflow was defined as 2-complements wrap-around.  When comparing
 * 'enumval - 1 == TARG_INT_MAX', there was another integer overflow, and
 * this one was optimized away by GCC, taking advantage of the undefined
 * behavior.
 */
enum int_limits {
	MAX_MINUS_2 = 0x7ffffffd,
	MAX_MINUS_1,
	MAX,
	/* expect+1: overflow in enumeration values: MIN [48] */
	MIN,
	MIN_PLUS_1
};
