/*	$NetBSD: platform_lp64.c,v 1.7 2023/07/08 12:45:43 rillig Exp $	*/
# 3 "platform_lp64.c"

/*
 * Test features that only apply to platforms that have 32-bit int and 64-bit
 * long and pointer types.
 */

/* lint1-only-if: lp64 */
/* lint1-extra-flags: -c -h -a -p -b -r -z -X 351 */

int s32;
unsigned int u32;
long sl32;
unsigned long ul32;
__int128_t s128;
__uint128_t u128;

void
convert_between_int_and_long(void)
{
	/* expect+1: warning: conversion from 'long' to 'int' may lose accuracy [132] */
	s32 = sl32;
	sl32 = s32;
	/* expect+1: warning: conversion from 'unsigned long' to 'unsigned int' may lose accuracy [132] */
	u32 = ul32;
	ul32 = u32;
}

void to_size_t(typeof(sizeof(int)));

void
convert_unsigned_char_to_size_t(unsigned char uc)
{
	/* no warning, unlike platform_int */
	to_size_t(uc);
}

void
convert_128(void)
{
	/* expect+1: warning: conversion from '__int128_t' to 'int' may lose accuracy [132] */
	s32 = s128;
	/* expect+1: warning: conversion from '__uint128_t' to 'unsigned int' may lose accuracy [132] */
	u32 = u128;
}
