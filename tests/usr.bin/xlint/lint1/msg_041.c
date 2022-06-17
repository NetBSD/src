/*	$NetBSD: msg_041.c,v 1.5 2022/06/17 18:54:53 rillig Exp $	*/
# 3 "msg_041.c"

// Test for message: bit-field in union is very unusual [41]

union u {
	int member;
	/* expect+1: warning: bit-field in union is very unusual [41] */
	unsigned bit_field : 7;
};
