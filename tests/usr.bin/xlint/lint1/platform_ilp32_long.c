/*	$NetBSD: platform_ilp32_long.c,v 1.2 2023/03/28 14:44:35 rillig Exp $	*/
# 3 "platform_ilp32_long.c"

/*
 * Test features that only apply to platforms that have 32-bit int, long and
 * pointer types and where size_t is unsigned long, not unsigned int.
 *
 * On these platforms, in portable mode (-p), the type 'int' is in some cases
 * assumed to be only 24 bits wide, in order to detect conversions from
 * 'long' (or more probably 'size_t') to 'int', which can lose accuracy.
 */

/* lint1-only-if: ilp32 long */
/* lint1-extra-flags: -c -h -a -p -b -r -z -X 351 */

int s32;
unsigned int u32;
long sl32;
unsigned long ul32;

void
convert_between_int_and_long(void)
{
	/*
	 * Even though 'long' and 'int' have the same size on this platform,
	 * the option '-p' enables additional portability checks that assume
	 * a 24-bit int and a 32-bit long type, to proactively detect loss of
	 * accuracy on potential other platforms.
	 */
	/* expect+1: warning: conversion from 'long' to 'int' may lose accuracy [132] */
	s32 = sl32;
	sl32 = s32;
	u32 = ul32;
	ul32 = u32;
}
