/*	$NetBSD: platform_ldbl128.c,v 1.6 2023/07/05 11:42:14 rillig Exp $	*/
# 3 "platform_ldbl128.c"

/*
 * Test features that only apply to platforms that have 128-bit long double.
 */

/* lint1-extra-flags: -c -h -a -p -b -r -z -X 351 */
/* lint1-only-if: ldbl128 */

/* CONSTCOND */
typedef int bits_per_byte[((unsigned char)-1) == 255 ? 1 : -1];
typedef int bytes_per_long_double[sizeof(long double) == 16 ? 1 : -1];

/*
 * Platforms with 128-bit 'long double' typically use IEEE 754-2008, which has
 * 1 bit sign + 15 bit exponent + 112 bit normalized mantissa. This means the
 * maximum representable value is 1.1111111(bin) * 2^16383, which is about
 * 1.189e4932. This is in the same range as for 96-bit 'long double', as the
 * exponent range is the same.
 */
/* expect+1: warning: floating-point constant out of range [248] */
double larger_than_ldbl = 1e4933;
/* expect+1: warning: floating-point constant out of range [248] */
long double larger_than_ldbl_l = 1e4933L;
/* expect+1: warning: floating-point constant out of range [248] */
double larger_than_dbl = 1e4932;
/* Fits in 'long double' but not in 'double'. */
long double larger_than_dbl_l = 1e4932L;
