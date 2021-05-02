/*	$NetBSD: gcc_bit_field_types.c,v 1.1 2021/05/02 21:22:09 rillig Exp $	*/
# 3 "gcc_bit_field_types.c"

/*
 * https://gcc.gnu.org/onlinedocs/gcc/Structures-unions-enumerations-and-bit-fields-implementation.html
 *
 * "Other integer types, such as long int, and enumerated types are permitted
 * even in strictly conforming mode."
 *
 * See msg_035.c.
 */

// Test for message: illegal bit-field type '%s' [35]

/* Omit -g, see gcc_bit_field_types.c. */
/* lint1-flags: -Sw */

/* Try all types from tspec_t. */
struct example {
	int int_flag: 1;
	unsigned int unsigned_int_flag: 1;
	long long_flag: 1;	/* expect: 35 *//*FIXME*/
	unsigned long unsigned_long_flag: 1;	/* expect: 35 *//*FIXME*/
	long long long_long_flag: 1;	/* expect: 35 *//*FIXME*/
	unsigned long long unsigned_long_long_flag: 1;	/* expect: 35 *//*FIXME*/
	double double_flag: 1;	/* expect: 35 */
};
