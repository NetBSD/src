/*	$NetBSD: gcc_bit_field_types.c,v 1.3 2021/05/02 22:07:49 rillig Exp $	*/
# 3 "gcc_bit_field_types.c"

/*
 * https://gcc.gnu.org/onlinedocs/gcc/Structures-unions-enumerations-and-bit-fields-implementation.html
 *
 * "Other integer types, such as long int, and enumerated types are permitted
 * even in strictly conforming mode."
 *
 * See msg_035.c.
 */

struct example {
	int int_flag: 1;
	unsigned int unsigned_int_flag: 1;
	long long_flag: 1;
	unsigned long unsigned_long_flag: 1;
	long long long_long_flag: 1;
	unsigned long long unsigned_long_long_flag: 1;
	double double_flag: 1;	/* expect: illegal bit-field type 'double' */
};
