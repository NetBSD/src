/*	$NetBSD: msg_273.c,v 1.4 2023/08/26 10:43:53 rillig Exp $	*/
# 3 "msg_273.c"

/* Test for message: bit-field type '%s' invalid in C90 or later [273] */

/* lint1-flags: -sw */

struct bit_fields {
	int plain_int: 3;
	unsigned int unsigned_int: 3;
	signed int signed_int: 3;
	/* expect+1: warning: bit-field type 'unsigned char' invalid in C90 or later [273] */
	unsigned char unsigned_char: 3;
};
