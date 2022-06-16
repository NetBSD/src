/*	$NetBSD: msg_221.c,v 1.5 2022/06/16 21:24:41 rillig Exp $	*/
# 3 "msg_221.c"

// Test for message: initialization of unsigned with negative constant [221]

struct example {
	unsigned int a: 5;
	unsigned int b: 5;
} example_var = {
    /* expect+1: warning: initialization of unsigned with negative constant [221] */
    -1,
    31
};
