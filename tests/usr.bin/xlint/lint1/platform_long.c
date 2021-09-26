/*	$NetBSD: platform_long.c,v 1.1 2021/09/26 03:17:59 rillig Exp $	*/
# 3 "platform_long.c"

/*
 * Test features that only apply to platforms on which size_t is unsigned
 * long and ptr_diff is signed long.
 */

/* lint1-extra-flags: -h */
/* lint1-only-if: long */

void to_size(typeof(sizeof(int)));

void
convert_unsigned_char_to_size(unsigned char uc)
{
	/* no warning, unlike in platform_int */
	to_size(uc);
}

/* expect+1: warning: static variable unused_variable unused [226] */
static int unused_variable;

/*
 * XXX: On 2021-09-23, the releng build failed on i386 but not on sparc.
 * usr.bin/make/cond.c, call to is_token with unsigned char as third argument.
 * Based on that, this test should succeed on sparc, but with a cross-compiled
 * lint on x86_64 with ARCHSUBDIR=sparc, it failed.
 */
