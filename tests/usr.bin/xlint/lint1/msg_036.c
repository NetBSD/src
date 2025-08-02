/*	$NetBSD: msg_036.c,v 1.4.4.1 2025/08/02 05:58:15 perseant Exp $	*/
# 3 "msg_036.c"

// Test for message: invalid bit-field size: %d [36]

struct example {
	/* expect+1: error: invalid bit-field size: 160 [36] */
	unsigned int too_large: 100000;
	/* expect+1: error: invalid bit-field size: 255 [36] */
	unsigned int negative: -1;
	unsigned int ok: 3;
};
