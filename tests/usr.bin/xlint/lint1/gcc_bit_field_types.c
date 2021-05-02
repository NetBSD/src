/*	$NetBSD: gcc_bit_field_types.c,v 1.2 2021/05/02 21:47:28 rillig Exp $	*/
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
	long long_flag: 1;	/* expect: 35 *//*FIXME*/
	unsigned long unsigned_long_flag: 1;	/* expect: 35 *//*FIXME*/
	long long long_long_flag: 1;	/* expect: 35 *//*FIXME*/
	unsigned long long unsigned_long_long_flag: 1;	/* expect: 35 *//*FIXME*/
	double double_flag: 1;	/* expect: 35 */
};
