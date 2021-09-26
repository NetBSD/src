/*	$NetBSD: platform_int.c,v 1.2 2021/09/26 14:28:22 rillig Exp $	*/
# 3 "platform_int.c"

/*
 * Test features that only apply to platforms on which size_t is unsigned
 * int and ptr_diff is signed int.
 */

/* lint1-extra-flags: -c -h -a -p -b -r -z */
/* lint1-only-if: int */

void to_size(typeof(sizeof(int)));

void
convert_unsigned_char_to_size(unsigned char uc)
{
	/* expect+1: warning: argument #1 is converted from 'unsigned char' to 'unsigned int' due to prototype [259] */
	to_size(uc);
}
