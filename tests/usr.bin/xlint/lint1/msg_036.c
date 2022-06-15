/*	$NetBSD: msg_036.c,v 1.4 2022/06/15 20:18:31 rillig Exp $	*/
# 3 "msg_036.c"

// Test for message: illegal bit-field size: %d [36]

struct example {
	/* expect+1: error: illegal bit-field size: 160 [36] */
	unsigned int too_large: 100000;
	/* expect+1: error: illegal bit-field size: 255 [36] */
	unsigned int negative: -1;
	unsigned int ok: 3;
};
