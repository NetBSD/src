/*	$NetBSD: msg_180.c,v 1.5 2023/03/28 14:44:35 rillig Exp $	*/
# 3 "msg_180.c"

// Test for message: bit-field initializer does not fit [180]

/* lint1-extra-flags: -X 351 */

struct example {
	unsigned int a: 5;
	unsigned int b: 5;
} example_var = {
    /* expect+1: warning: bit-field initializer does not fit [180] */
    32,
    31
};
