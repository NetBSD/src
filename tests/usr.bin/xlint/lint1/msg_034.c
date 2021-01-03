/*	$NetBSD: msg_034.c,v 1.2 2021/01/03 15:35:00 rillig Exp $	*/
# 3 "msg_034.c"

// Test for message: nonportable bit-field type [34]

/* lint1-flags: -S -g -p -w */

struct example {
	int nonportable: 1;
	unsigned int portable: 1;
};
