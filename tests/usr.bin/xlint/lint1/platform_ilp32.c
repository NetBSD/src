/*	$NetBSD: platform_ilp32.c,v 1.6 2024/06/08 06:37:06 rillig Exp $	*/
# 3 "platform_ilp32.c"

/*
 * Test features that only apply to platforms that have 32-bit int, long and
 * pointer types.
 *
 * See also:
 *	platform_ilp32_int.c
 *	platform_ilp32_long.c
 */

/* lint1-extra-flags: -c -h -a -p -b -r -z -X 351 */
/* lint1-only-if: ilp32 */

void
switch_s64(long long x)
{
	switch (x) {
	case 0x222200000001:
	case 0x333300000001:
	/* expect+1: error: duplicate case '37529424232449' in switch [199] */
	case 0x222200000001:
	case -0x7fffffffffffffff:
	/* expect+1: error: duplicate case '-9223372036854775807' in switch [199] */
	case -0x7fffffffffffffff:
		break;
	}
}

void
switch_u64(unsigned long long x)
{
	switch (x) {
	case 0x222200000001:
	case 0x333300000001:
	/* expect+1: error: duplicate case '37529424232449' in switch [200] */
	case 0x222200000001:
	/* expect+1: warning: conversion of negative constant -9223372036854775807 to unsigned type 'unsigned long long' [222] */
	case -0x7fffffffffffffff:
	/* expect+2: warning: conversion of negative constant -9223372036854775807 to unsigned type 'unsigned long long' [222] */
	/* expect+1: error: duplicate case '9223372036854775809' in switch [200] */
	case -0x7fffffffffffffff:
		break;
	}
}
