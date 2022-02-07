/*	$NetBSD: msg_041.c,v 1.3 2022/02/07 02:39:10 rillig Exp $	*/
# 3 "msg_041.c"

// Test for message: illegal use of bit-field [41]

union u {
	int member;
	/* expect+1: illegal use of bit-field [41] */
	unsigned bit_field : 7;
};
