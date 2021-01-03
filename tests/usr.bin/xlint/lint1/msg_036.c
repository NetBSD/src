/*	$NetBSD: msg_036.c,v 1.2 2021/01/03 15:35:00 rillig Exp $	*/
# 3 "msg_036.c"

// Test for message: illegal bit-field size: %d [36]

struct example {
	unsigned int too_large: 100000;
	unsigned int negative: -1;
	unsigned int ok: 3;
};
