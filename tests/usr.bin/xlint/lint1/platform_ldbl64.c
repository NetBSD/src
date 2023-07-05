/*	$NetBSD: platform_ldbl64.c,v 1.5 2023/07/05 11:42:14 rillig Exp $	*/
# 3 "platform_ldbl64.c"

/*
 * Test features that only apply to platforms that have 64-bit long double.
 */

/* lint1-extra-flags: -c -h -a -p -b -r -z -X 351 */
/* lint1-only-if: ldbl64 */

/* CONSTCOND */
typedef int bits_per_byte[((unsigned char)-1) == 255 ? 1 : -1];
typedef int bytes_per_long_double[sizeof(long double) == 8 ? 1 : -1];

/* expect+1: warning: floating-point constant out of range [248] */
double larger_than_ldbl = 1e310;
/*
 * Since 'long double' has the same size as 'double', there is no floating
 * point constant that fits in 'long double' but not in 'double'.
 */
/* expect+1: warning: floating-point constant out of range [248] */
long double larger_than_ldbl_l = 1e310L;
