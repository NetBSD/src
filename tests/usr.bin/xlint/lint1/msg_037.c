/*	$NetBSD: msg_037.c,v 1.2 2021/01/03 15:35:00 rillig Exp $	*/
# 3 "msg_037.c"

// Test for message: zero size bit-field [37]

struct example {
	unsigned int zero: 0;
	unsigned int ok: 3;
};
