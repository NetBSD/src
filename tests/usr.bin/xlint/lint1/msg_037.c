/*	$NetBSD: msg_037.c,v 1.4 2022/06/15 20:18:31 rillig Exp $	*/
# 3 "msg_037.c"

// Test for message: zero size bit-field [37]

struct example {
	/* expect+1: error: zero size bit-field [37] */
	unsigned int zero: 0;
	unsigned int ok: 3;
};
