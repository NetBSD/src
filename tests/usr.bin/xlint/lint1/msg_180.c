/*	$NetBSD: msg_180.c,v 1.2 2021/01/07 18:37:41 rillig Exp $	*/
# 3 "msg_180.c"

// Test for message: bit-field initializer does not fit [180]

struct example {
	unsigned int a: 5;
	unsigned int b: 5;
} example_var = {
    32,
    31
};
