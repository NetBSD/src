/*	$NetBSD: gcc_bit_field_types.c,v 1.6 2022/01/15 14:22:03 rillig Exp $	*/
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
	/* expect+1: warning: illegal bit-field type 'double' [35] */
	double double_flag: 1;
};

struct large_bit_field {
	unsigned long long member: 48;
};

unsigned long long
promote_large_bit_field(struct large_bit_field lbf)
{
	/*
	 * Before tree.c 1.281 from 2021-05-04:
	 * lint: assertion "len == size_in_bits(INT)" failed
	 *     in promote at tree.c:1698
	 */
	return lbf.member & 0xf;
}
