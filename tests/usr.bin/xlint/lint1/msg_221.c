/*	$NetBSD: msg_221.c,v 1.2 2021/01/07 18:37:41 rillig Exp $	*/
# 3 "msg_221.c"

// Test for message: initialisation of unsigned with negative constant [221]

struct example {
	unsigned int a: 5;
	unsigned int b: 5;
} example_var = {
    -1,
    31
};
