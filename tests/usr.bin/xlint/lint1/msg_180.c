/*	$NetBSD: msg_180.c,v 1.4 2022/06/16 16:58:36 rillig Exp $	*/
# 3 "msg_180.c"

// Test for message: bit-field initializer does not fit [180]

struct example {
	unsigned int a: 5;
	unsigned int b: 5;
} example_var = {
    /* expect+1: warning: bit-field initializer does not fit [180] */
    32,
    31
};
