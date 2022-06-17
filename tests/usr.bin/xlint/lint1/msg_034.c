/*	$NetBSD: msg_034.c,v 1.6 2022/06/17 18:54:53 rillig Exp $	*/
# 3 "msg_034.c"

// Test for message: nonportable bit-field type '%s' [34]

/* No -g since GCC allows all integer types as bit-fields. */
/* lint1-flags: -S -p -w */

/*
 * C90 3.5.2.1 allows 'int', 'signed int', 'unsigned int' as bit-field types.
 *
 * C99 6.7.2.1 significantly changed the wording of the allowable types for
 * bit-fields.  For example, 6.7.2.1p4 does not mention plain 'int' at all.
 * The rationale for C99 6.7.2.1 mentions plain int though, and it would have
 * broken a lot of existing code to disallow plain 'int' as a bit-field type.
 * Footnote 104 explicitly mentions plain 'int' as well and it even allows
 * typedef-types for bit-fields.
 */
struct example {
	/* expect+1: warning: nonportable bit-field type 'unsigned short' [34] */
	unsigned short ushort: 1;

	/* expect+1: warning: bit-field of type plain 'int' has implementation-defined signedness [344] */
	int plain_int: 1;

	signed int signed_int: 1;
	unsigned int portable: 1;
};
