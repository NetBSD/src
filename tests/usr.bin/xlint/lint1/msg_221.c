/*	$NetBSD: msg_221.c,v 1.7 2024/06/08 06:37:06 rillig Exp $	*/
# 3 "msg_221.c"

// Test for message: initialization of unsigned type '%s' with negative constant %lld [221]

/* lint1-extra-flags: -X 351 */

struct example {
	unsigned int a: 5;
	unsigned int b: 5;
} example_var = {
    /* expect+1: warning: initialization of unsigned type 'unsigned int:5' with negative constant -1 [221] */
    -1,
    31
};
