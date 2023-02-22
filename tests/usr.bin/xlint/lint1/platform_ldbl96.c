/*	$NetBSD: platform_ldbl96.c,v 1.3 2023/02/22 22:12:35 rillig Exp $	*/
# 3 "platform_ldbl96.c"

/*
 * Test features that only apply to platforms that have 96-bit long double.
 */

/* lint1-extra-flags: -c -h -a -p -b -r -z */
/* lint1-only-if: ldbl-96 */

/* CONSTCOND */
typedef int bits_per_byte[((unsigned char)-1) == 255 ? 1 : -1];
typedef int bytes_per_long_double[sizeof(long double) == 12 ? 1 : -1];
