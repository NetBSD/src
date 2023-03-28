/*	$NetBSD: msg_221.c,v 1.6 2023/03/28 14:44:35 rillig Exp $	*/
# 3 "msg_221.c"

// Test for message: initialization of unsigned with negative constant [221]

/* lint1-extra-flags: -X 351 */

struct example {
	unsigned int a: 5;
	unsigned int b: 5;
} example_var = {
    /* expect+1: warning: initialization of unsigned with negative constant [221] */
    -1,
    31
};
