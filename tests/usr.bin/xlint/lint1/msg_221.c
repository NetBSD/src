/*	$NetBSD: msg_221.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_221.c"

// Test for message: initialisation of unsigned with negative constant [221]

struct example {
	unsigned int a: 5;
	unsigned int b: 5;
} example_var = {
    -1,				/* expect: 221 */
    31
};
