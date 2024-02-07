/*	$NetBSD: platform_ilp32_trad.c,v 1.4 2024/02/07 22:59:28 rillig Exp $	*/
# 3 "platform_ilp32_trad.c"

/*
 * Tests that are specific to ILP32 platforms and traditional C.
 */

/* lint1-flags: -tw -X 351 */
/* lint1-only-if: ilp32 */

void *lex_integer[] = {
	/* expect+1: ... integer 'int' ... */
	2147483647,
	/* expect+1: ... integer 'int' ... */
	0x7fffffff,
	/* expect+2: warning: integer constant out of range [252] */
	/* expect+1: ... integer 'long' ... */
	2147483648,
	/* expect+1: ... integer 'long' ... */
	0x80000000,
	/* expect+2: warning: integer constant out of range [252] */
	/* expect+1: ... integer 'long' ... */
	4294967295,
	/* expect+1: ... integer 'long' ... */
	0xffffffff,
	/* expect+1: warning: integer constant out of range [252] */
	4294967296,
	/* expect+1: warning: integer constant out of range [252] */
	0x0000000100000000,
	/* expect+2: warning: integer constant out of range [252] */
	/* expect+1: ... integer 'long' ... */
	9223372036854775807,
	/* expect+2: warning: integer constant out of range [252] */
	/* expect+1: ... integer 'long' ... */
	0x7fffffffffffffff,
	/* expect+1: warning: integer constant out of range [252] */
	9223372036854775808,
	/* expect+1: warning: integer constant out of range [252] */
	0x8000000000000000,
	/* expect+2: warning: integer constant out of range [252] */
	/* expect+1: ... integer 'long' ... */
	18446744073709551615,
	/* expect+2: warning: integer constant out of range [252] */
	/* expect+1: ... integer 'long' ... */
	0xffffffffffffffff,
	/* expect+2: warning: integer constant out of range [252] */
	/* expect+1: ... integer 'long' ... */
	18446744073709551616,
	/* expect+2: warning: integer constant out of range [252] */
	/* expect+1: ... integer 'long' ... */
	0x00010000000000000000,

	/* expect+1: ... integer 'long' ... */
	2147483647L,
	/* expect+1: ... integer 'long' ... */
	0x7fffffffL,
	/* expect+2: warning: integer constant out of range [252] */
	/* expect+1: ... integer 'long' ... */
	2147483648L,
	/* expect+1: ... integer 'long' ... */
	0x80000000L,
	/* expect+2: warning: integer constant out of range [252] */
	/* expect+1: ... integer 'long' ... */
	4294967295L,
	/* expect+1: ... integer 'long' ... */
	0xffffffffL,
	/* expect+1: warning: integer constant out of range [252] */
	4294967296L,
	/* expect+1: warning: integer constant out of range [252] */
	0x0000000100000000L,
	/* expect+2: warning: integer constant out of range [252] */
	/* expect+1: ... integer 'long' ... */
	9223372036854775807L,
	/* expect+2: warning: integer constant out of range [252] */
	/* expect+1: ... integer 'long' ... */
	0x7fffffffffffffffL,
	/* expect+1: warning: integer constant out of range [252] */
	9223372036854775808L,
	/* expect+1: warning: integer constant out of range [252] */
	0x8000000000000000L,
	/* expect+2: warning: integer constant out of range [252] */
	/* expect+1: ... integer 'long' ... */
	18446744073709551615L,
	/* expect+2: warning: integer constant out of range [252] */
	/* expect+1: ... integer 'long' ... */
	0xffffffffffffffffL,
	/* expect+2: warning: integer constant out of range [252] */
	/* expect+1: ... integer 'long' ... */
	18446744073709551616L,
	/* expect+2: warning: integer constant out of range [252] */
	/* expect+1: ... integer 'long' ... */
	0x00010000000000000000L,

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
	/* expect+2: warning: integer constant out of range [252] */
	/* expect+1: ... integer 'long long' ... */
	9223372036854775808LL,
	/* expect+1: ... integer 'long long' ... */
	0x8000000000000000LL,
	/* expect+2: warning: integer constant out of range [252] */
	/* expect+1: ... integer 'long long' ... */
	18446744073709551615LL,
	/* expect+1: ... integer 'long long' ... */
	0xffffffffffffffffLL,
	/* expect+2: warning: integer constant out of range [252] */
	/* expect+1: ... integer 'long long' ... */
	18446744073709551616LL,
	/* expect+2: warning: integer constant out of range [252] */
	/* expect+1: ... integer 'long long' ... */
	0x00010000000000000000LL,
};
