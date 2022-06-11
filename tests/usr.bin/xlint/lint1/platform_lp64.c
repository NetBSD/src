/*	$NetBSD: platform_lp64.c,v 1.3 2022/06/11 11:52:13 rillig Exp $	*/
# 3 "platform_lp64.c"

/*
 * Test features that only apply to platforms that have 32-bit int and 64-bit
 * long and pointer types.
 */

/* lint1-extra-flags: -c -h -a -p -b -r -z */
/* lint1-only-if: lp64 */

void to_size_t(typeof(sizeof(int)));

void
convert_unsigned_char_to_size_t(unsigned char uc)
{
	/* no warning, unlike platform_int */
	to_size_t(uc);
}

/* expect+1: warning: static variable 'unused_variable' unused [226] */
static int unused_variable;
