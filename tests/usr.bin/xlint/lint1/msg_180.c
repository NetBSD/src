/*	$NetBSD: msg_180.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_180.c"

// Test for message: bit-field initializer does not fit [180]

struct example {
	unsigned int a: 5;
	unsigned int b: 5;
} example_var = {
    32,				/* expect: 180 */
    31
};
