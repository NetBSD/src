/*	$NetBSD: platform_long.c,v 1.2 2021/09/26 14:28:22 rillig Exp $	*/
# 3 "platform_long.c"

/*
 * Test features that only apply to platforms on which size_t is unsigned
 * long and ptr_diff is signed long.
 */

/* lint1-extra-flags: -c -h -a -p -b -r -z */
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
