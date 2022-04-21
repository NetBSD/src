/*	$NetBSD: msg_132.c,v 1.9 2022/04/21 19:48:18 rillig Exp $	*/
# 3 "msg_132.c"

// Test for message: conversion from '%s' to '%s' may lose accuracy [132]

/*
 * NetBSD's default lint flags only include a single -a, which only flags
 * narrowing conversions from long.  To get warnings for all narrowing
 * conversions, -a needs to be given more than once.
 *
 * https://gnats.netbsd.org/14531
 */

/* lint1-extra-flags: -aa */

unsigned char u8;
unsigned short u16;
unsigned int u32;
unsigned long long u64;

signed char s8;
signed short s16;
signed int s32;
signed long long s64;

void
unsigned_to_unsigned(void)
{
	u8 = u16;		/* expect: 132 */
	u8 = u32;		/* expect: 132 */
	u8 = u64;		/* expect: 132 */

	u16 = u8;
	u16 = u32;		/* expect: 132 */
	u16 = u64;		/* expect: 132 */

	u32 = u8;
	u32 = u16;
	u32 = u64;		/* expect: 132 */

	u64 = u8;
	u64 = u16;
	u64 = u32;
}

void
unsigned_to_signed(void)
{
	s8 = u16;		/* expect: 132 */
	s8 = u32;		/* expect: 132 */
	s8 = u64;		/* expect: 132 */

	s16 = u8;
	s16 = u32;		/* expect: 132 */
	s16 = u64;		/* expect: 132 */

	s32 = u8;
	s32 = u16;
	s32 = u64;		/* expect: 132 */

	s64 = u8;
	s64 = u16;
	s64 = u32;
}

void
signed_to_unsigned(void)
{
	u8 = s16;		/* expect: 132 */
	u8 = s32;		/* expect: 132 */
	u8 = s64;		/* expect: 132 */

	u16 = s8;
	u16 = s32;		/* expect: 132 */
	u16 = s64;		/* expect: 132 */

	u32 = s8;
	u32 = s16;
	u32 = s64;		/* expect: 132 */

	u64 = s8;
	u64 = s16;
	u64 = s32;
}

void
signed_to_signed(void)
{
	s8 = s16;		/* expect: 132 */
	s8 = s32;		/* expect: 132 */
	s8 = s64;		/* expect: 132 */

	s16 = s8;
	s16 = s32;		/* expect: 132 */
	s16 = s64;		/* expect: 132 */

	s32 = s8;
	s32 = s16;
	s32 = s64;		/* expect: 132 */

	s64 = s8;
	s64 = s16;
	s64 = s32;
}

/*
 * Before tree.c 1.268 from 2021-04-06, lint wrongly warned that conversion
 * to _Bool might lose accuracy.  C99 6.3.1.2 defines a special conversion
 * rule from scalar to _Bool though by comparing the value to 0.
 */
_Bool
to_bool(long a, long b)
{
	/* seen in fp_lib.h, function wideRightShiftWithSticky */
	return a | b;
}

/* ARGSUSED */
const char *
cover_build_plus_minus(const char *arr, double idx)
{
	/* expect+3: error: operands of '+' have incompatible types (pointer != double) [107] */
	/* expect+2: warning: function 'cover_build_plus_minus' expects to return value [214] */
	if (idx > 0.0)
		return arr + idx;
	return arr + (unsigned int)idx;
}

int
non_constant_expression(void)
{
	/*
	 * Even though this variable definition looks like a constant, it
	 * does not fall within C's definition of an integer constant
	 * expression.  Due to that, lint does not perform constant folding
	 * on the expression built from this variable and thus doesn't know
	 * that the conversion will always succeed.
	 */
	const int not_a_constant = 8;
	/* expect+1: warning: conversion from 'unsigned long long' to 'int' may lose accuracy [132] */
	return not_a_constant * 8ULL;
}
