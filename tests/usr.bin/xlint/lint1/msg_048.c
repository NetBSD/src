/*	$NetBSD: msg_048.c,v 1.3 2022/04/08 21:29:29 rillig Exp $	*/
# 3 "msg_048.c"

// Test for message: overflow in enumeration values: %s [48]

/*
 * Before decl.c 1.TODO from 2022-04-TODO, the comparison for enum constant
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
	/* TODO: expect: overflow in enumeration values: MIN */
	MIN
};

TODO: "Add example code that triggers the above message." /* expect: 249 */
TODO: "Add example code that almost triggers the above message."
