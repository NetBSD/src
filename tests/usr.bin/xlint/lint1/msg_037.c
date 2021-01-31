/*	$NetBSD: msg_037.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_037.c"

// Test for message: zero size bit-field [37]

struct example {
	unsigned int zero: 0;	/* expect: 37 */
	unsigned int ok: 3;
};
