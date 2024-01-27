/*	$NetBSD: platform_lp64_c90.c,v 1.1 2024/01/27 15:10:57 rillig Exp $	*/
# 3 "platform_lp64_c90.c"

/*
 * Tests that are specific to LP64 platforms and the language level C90.
 */

/* lint1-flags: -sw -X 351 */
/* lint1-only-if: lp64 */

void *lex_integer[] = {
	/* expect+1: ... integer 'int' ... */
	2147483647,
	/* expect+1: ... integer 'int' ... */
	0x7fffffff,
	/* expect+1: ... integer 'long' ... */
	2147483648,
	/* expect+1: ... integer 'unsigned int' ... */
	0x80000000,
	/* expect+1: ... integer 'long' ... */
	4294967295,
	/* expect+1: ... integer 'unsigned int' ... */
	0xffffffff,
	/* expect+1: ... integer 'long' ... */
	4294967296,
	/* expect+1: ... integer 'long' ... */
	0x0000000100000000,
	/* expect+1: ... integer 'long' ... */
	9223372036854775807,
	/* expect+1: ... integer 'long' ... */
	0x7fffffffffffffff,
	/* expect+1: ... integer 'unsigned long' ... */
	9223372036854775808,
	/* expect+1: ... integer 'unsigned long' ... */
	0x8000000000000000,
	/* expect+1: ... integer 'unsigned long' ... */
	18446744073709551615,
	/* expect+1: ... integer 'unsigned long' ... */
	0xffffffffffffffff,
	/* expect+2: ... integer 'unsigned long' ... */
	/* expect+1: warning: integer constant out of range [252] */
	18446744073709551616,
	/* expect+2: ... integer 'unsigned long' ... */
	/* expect+1: warning: integer constant out of range [252] */
	0x00010000000000000000,

	/* expect+1: ... integer 'unsigned int' ... */
	2147483647U,
	/* expect+1: ... integer 'unsigned int' ... */
	0x7fffffffU,
	/* expect+1: ... integer 'unsigned int' ... */
	2147483648U,
	/* expect+1: ... integer 'unsigned int' ... */
	0x80000000U,
	/* expect+1: ... integer 'unsigned int' ... */
	4294967295U,
	/* expect+1: ... integer 'unsigned int' ... */
	0xffffffffU,
	/* expect+1: ... integer 'unsigned long' ... */
	4294967296U,
	/* expect+1: ... integer 'unsigned long' ... */
	0x0000000100000000U,
	/* expect+1: ... integer 'unsigned long' ... */
	9223372036854775807U,
	/* expect+1: ... integer 'unsigned long' ... */
	0x7fffffffffffffffU,
	/* expect+1: ... integer 'unsigned long' ... */
	9223372036854775808U,
	/* expect+1: ... integer 'unsigned long' ... */
	0x8000000000000000U,
	/* expect+1: ... integer 'unsigned long' ... */
	18446744073709551615U,
	/* expect+1: ... integer 'unsigned long' ... */
	0xffffffffffffffffU,
	/* expect+2: ... integer 'unsigned long' ... */
	/* expect+1: warning: integer constant out of range [252] */
	18446744073709551616U,
	/* expect+2: ... integer 'unsigned long' ... */
	/* expect+1: warning: integer constant out of range [252] */
	0x00010000000000000000U,

	/* expect+1: ... integer 'long' ... */
	2147483647L,
	/* expect+1: ... integer 'long' ... */
	0x7fffffffL,
	/* expect+1: ... integer 'long' ... */
	2147483648L,
	/* expect+1: ... integer 'long' ... */
	0x80000000L,
	/* expect+1: ... integer 'long' ... */
	4294967295L,
	/* expect+1: ... integer 'long' ... */
	0xffffffffL,
	/* expect+1: ... integer 'long' ... */
	4294967296L,
	/* expect+1: ... integer 'long' ... */
	0x0000000100000000L,
	/* expect+1: ... integer 'long' ... */
	9223372036854775807L,
	/* expect+1: ... integer 'long' ... */
	0x7fffffffffffffffL,
	/* expect+1: ... integer 'unsigned long' ... */
	9223372036854775808L,
	/* expect+1: ... integer 'unsigned long' ... */
	0x8000000000000000L,
	/* expect+1: ... integer 'unsigned long' ... */
	18446744073709551615L,
	/* expect+1: ... integer 'unsigned long' ... */
	0xffffffffffffffffL,
	/* expect+2: warning: integer constant out of range [252] */
	/* expect+1: ... integer 'unsigned long' ... */
	18446744073709551616L,
	/* expect+2: ... integer 'unsigned long' ... */
	/* expect+1: warning: integer constant out of range [252] */
	0x00010000000000000000L,

	/* expect+1: ... integer 'unsigned long' ... */
	2147483647UL,
	/* expect+1: ... integer 'unsigned long' ... */
	0x7fffffffUL,
	/* expect+1: ... integer 'unsigned long' ... */
	2147483648UL,
	/* expect+1: ... integer 'unsigned long' ... */
	0x80000000UL,
	/* expect+1: ... integer 'unsigned long' ... */
	4294967295UL,
	/* expect+1: ... integer 'unsigned long' ... */
	0xffffffffUL,
	/* expect+1: ... integer 'unsigned long' ... */
	4294967296UL,
	/* expect+1: ... integer 'unsigned long' ... */
	0x0000000100000000UL,
	/* expect+1: ... integer 'unsigned long' ... */
	9223372036854775807UL,
	/* expect+1: ... integer 'unsigned long' ... */
	0x7fffffffffffffffUL,
	/* expect+1: ... integer 'unsigned long' ... */
	9223372036854775808UL,
	/* expect+1: ... integer 'unsigned long' ... */
	0x8000000000000000UL,
	/* expect+1: ... integer 'unsigned long' ... */
	18446744073709551615UL,
	/* expect+1: ... integer 'unsigned long' ... */
	0xffffffffffffffffUL,
	/* expect+2: ... integer 'unsigned long' ... */
	/* expect+1: warning: integer constant out of range [252] */
	18446744073709551616UL,
	/* expect+2: ... integer 'unsigned long' ... */
	/* expect+1: warning: integer constant out of range [252] */
	0x00010000000000000000UL,

	/* expect+1: ... integer 'long long' ... */
	2147483647LL,
	/* expect+1: ... integer 'long long' ... */
	0x7fffffffLL,
	/* expect+1: ... integer 'long long' ... */
	2147483648LL,
	/* expect+1: ... integer 'long long' ... */
	0x80000000LL,
	/* expect+1: ... integer 'long long' ... */
	4294967295LL,
	/* expect+1: ... integer 'long long' ... */
	0xffffffffLL,
	/* expect+1: ... integer 'long long' ... */
	4294967296LL,
	/* expect+1: ... integer 'long long' ... */
	0x0000000100000000LL,
	/* expect+1: ... integer 'long long' ... */
	9223372036854775807LL,
	/* expect+1: ... integer 'long long' ... */
	0x7fffffffffffffffLL,
	/* expect+1: ... integer 'unsigned long long' ... */
	9223372036854775808LL,
	/* expect+1: ... integer 'unsigned long long' ... */
	0x8000000000000000LL,
	/* expect+1: ... integer 'unsigned long long' ... */
	18446744073709551615LL,
	/* expect+1: ... integer 'unsigned long long' ... */
	0xffffffffffffffffLL,
	/* expect+2: ... integer 'unsigned long long' ... */
	/* expect+1: warning: integer constant out of range [252] */
	18446744073709551616LL,
	/* expect+2: ... integer 'unsigned long long' ... */
	/* expect+1: warning: integer constant out of range [252] */
	0x00010000000000000000LL,

	/* expect+1: ... integer 'unsigned long long' ... */
	2147483647ULL,
	/* expect+1: ... integer 'unsigned long long' ... */
	0x7fffffffULL,
	/* expect+1: ... integer 'unsigned long long' ... */
	2147483648ULL,
	/* expect+1: ... integer 'unsigned long long' ... */
	0x80000000ULL,
	/* expect+1: ... integer 'unsigned long long' ... */
	4294967295ULL,
	/* expect+1: ... integer 'unsigned long long' ... */
	0xffffffffULL,
	/* expect+1: ... integer 'unsigned long long' ... */
	4294967296ULL,
	/* expect+1: ... integer 'unsigned long long' ... */
	0x0000000100000000ULL,
	/* expect+1: ... integer 'unsigned long long' ... */
	9223372036854775807ULL,
	/* expect+1: ... integer 'unsigned long long' ... */
	0x7fffffffffffffffULL,
	/* expect+1: ... integer 'unsigned long long' ... */
	9223372036854775808ULL,
	/* expect+1: ... integer 'unsigned long long' ... */
	0x8000000000000000ULL,
	/* expect+1: ... integer 'unsigned long long' ... */
	18446744073709551615ULL,
	/* expect+1: ... integer 'unsigned long long' ... */
	0xffffffffffffffffULL,
	/* expect+2: ... integer 'unsigned long long' ... */
	/* expect+1: warning: integer constant out of range [252] */
	18446744073709551616ULL,
	/* expect+2: warning: integer constant out of range [252] */
	/* expect+1: ... integer 'unsigned long long' ... */
	0x00010000000000000000ULL,
};
