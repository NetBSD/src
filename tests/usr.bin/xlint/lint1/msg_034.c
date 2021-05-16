/*	$NetBSD: msg_034.c,v 1.4 2021/05/16 10:34:19 rillig Exp $	*/
# 3 "msg_034.c"

// Test for message: nonportable bit-field type '%s' [34]

/* lint1-flags: -S -g -p -w */

struct example {
	int nonportable: 1;		/* expect: 34 */
	unsigned int portable: 1;
};
