/*	$NetBSD: msg_034.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_034.c"

// Test for message: nonportable bit-field type [34]

/* lint1-flags: -S -g -p -w */

struct example {
	int nonportable: 1;		/* expect: 34 */
	unsigned int portable: 1;
};
