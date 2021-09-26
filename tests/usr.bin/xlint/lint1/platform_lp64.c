/*	$NetBSD: platform_lp64.c,v 1.1 2021/09/26 03:17:59 rillig Exp $	*/
# 3 "platform_lp64.c"

/*
 * Test features that only apply to platforms that have 32-bit int and 64-bit
 * long and pointer types.
 */

/* lint1-only-if: lp64 */

void to_size_t(typeof(sizeof(int)));

void
convert_unsigned_char_to_size_t(unsigned char uc)
{
	/* no warning, unlike platform_int */
	to_size_t(uc);
}

/* expect+1: warning: static variable unused_variable unused [226] */
static int unused_variable;
