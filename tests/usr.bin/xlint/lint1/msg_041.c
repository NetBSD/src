/*	$NetBSD: msg_041.c,v 1.4 2022/02/07 02:44:49 rillig Exp $	*/
# 3 "msg_041.c"

// Test for message: bit-field in union is very unusual [41]

union u {
	int member;
	/* expect+1: bit-field in union is very unusual [41] */
	unsigned bit_field : 7;
};
