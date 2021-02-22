/*	$NetBSD: msg_221.c,v 1.4 2021/02/22 15:09:50 rillig Exp $	*/
# 3 "msg_221.c"

// Test for message: initialization of unsigned with negative constant [221]

struct example {
	unsigned int a: 5;
	unsigned int b: 5;
} example_var = {
    -1,				/* expect: 221 */
    31
};
