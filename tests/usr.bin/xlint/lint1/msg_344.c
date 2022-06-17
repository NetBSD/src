/*	$NetBSD: msg_344.c,v 1.2 2022/06/17 18:54:53 rillig Exp $	*/
# 3 "msg_344.c"

// Test for message: bit-field of type plain 'int' has implementation-defined signedness [344]

/* lint1-extra-flags: -p */

/*
 * C90 3.5.2.1 allows 'int', 'signed int', 'unsigned int' as bit-field types.
 *
 * C99 6.7.2.1 significantly changed the wording of the allowable types for
 * bit-fields.  For example, 6.7.2.1p4 does not mention plain 'int' at all.
 * The rationale for C99 6.7.2.1 mentions plain 'int' though, and it would
 * have broken a lot of existing code to disallow plain 'int' as a bit-field
 * type.  Footnote 104 explicitly mentions plain 'int' as well and it even
 * allows typedef-types for bit-fields.
 */
struct example {
	/* expect+1: warning: bit-field of type plain 'int' has implementation-defined signedness [344] */
	int plain_int: 1;

	signed int signed_int: 1;
	unsigned int unsigned_int: 1;
};
