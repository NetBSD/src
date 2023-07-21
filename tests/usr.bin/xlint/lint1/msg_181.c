/*	$NetBSD: msg_181.c,v 1.7 2023/07/21 06:02:07 rillig Exp $	*/
# 3 "msg_181.c"

// Test for message: {}-enclosed or constant initializer of type '%s' required [181]

/* lint1-extra-flags: -X 351 */

/* expect+1: error: {}-enclosed or constant initializer of type 'struct <unnamed>' required [181] */
struct { int x; } missing_braces = 3;
struct { int x; } including_braces = { 3 };


// C11 6.6p7 requires the initializer of an object with static storage duration
// to be a constant expression or an address constant, and a compound literal
// is neither.  C11 6.6p10 allows an implementation to accept "other forms of
// constant expressions", and GCC accepts compound literals that contain only
// constant expressions.
struct number {
	int value;
} num = (struct number){
    .value = 3,
};
/* expect-1: error: {}-enclosed or constant initializer of type 'struct number' required [181] */
