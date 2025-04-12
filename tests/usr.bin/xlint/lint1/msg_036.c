/*	$NetBSD: msg_036.c,v 1.5 2025/04/12 15:49:50 rillig Exp $	*/
# 3 "msg_036.c"

// Test for message: invalid bit-field size: %d [36]

struct example {
	/* expect+1: error: invalid bit-field size: 160 [36] */
	unsigned int too_large: 100000;
	/* expect+1: error: invalid bit-field size: 255 [36] */
	unsigned int negative: -1;
	unsigned int ok: 3;
};
