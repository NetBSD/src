/*	$NetBSD: msg_036.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_036.c"

// Test for message: illegal bit-field size: %d [36]

struct example {
	unsigned int too_large: 100000;	/* expect: 36 */
	unsigned int negative: -1;	/* expect: 36 */
	unsigned int ok: 3;
};
