/*	$NetBSD: msg_132.c,v 1.46 2024/10/12 09:45:26 rillig Exp $	*/
# 3 "msg_132.c"

// Test for message: conversion from '%s' to '%s' may lose accuracy [132]

/* lint1-extra-flags: -X 351 */

/*
 * NetBSD's default lint flags only include a single -a, which only flags
 * narrowing conversions from long.  To get warnings for all narrowing
 * conversions, -a needs to be given more than once.
 *
 * https://gnats.netbsd.org/14531
 */

/* lint1-extra-flags: -aa */

typedef unsigned char u8_t;
typedef unsigned short u16_t;
typedef unsigned int u32_t;
typedef unsigned long long u64_t;
typedef signed char s8_t;
typedef signed short s16_t;
typedef signed int s32_t;
typedef signed long long s64_t;

_Bool cond;
char ch;

u8_t u8;
u16_t u16;
u32_t u32;
u64_t u64;

s8_t s8;
s16_t s16;
s32_t s32;
s64_t s64;

struct bit_fields {
	unsigned u1:1;
	unsigned u2:2;
	unsigned u3:3;
	unsigned u4:4;
	unsigned u5:5;
	unsigned u6:6;
	unsigned u7:7;
	unsigned u8:8;
	unsigned u9:9;
	unsigned u10:10;
	unsigned u11:11;
	unsigned u12:12;
	unsigned u32:32;
} bits;


void
unsigned_to_unsigned(void)
{
	/* expect+1: warning: conversion from 'unsigned short' to 'unsigned char' may lose accuracy [132] */
	u8 = u16;
	/* expect+1: warning: conversion from 'unsigned int' to 'unsigned char' may lose accuracy [132] */
	u8 = u32;
	/* expect+1: warning: conversion from 'unsigned long long' to 'unsigned char' may lose accuracy [132] */
	u8 = u64;

	u16 = u8;
	/* expect+1: warning: conversion from 'unsigned int' to 'unsigned short' may lose accuracy [132] */
	u16 = u32;
	/* expect+1: warning: conversion from 'unsigned long long' to 'unsigned short' may lose accuracy [132] */
	u16 = u64;

	u32 = u8;
	u32 = u16;
	/* expect+1: warning: conversion from 'unsigned long long' to 'unsigned int' may lose accuracy [132] */
	u32 = u64;

	u64 = u8;
	u64 = u16;
	u64 = u32;
}

void
unsigned_to_signed(void)
{
	/* expect+1: warning: conversion from 'unsigned short' to 'signed char' may lose accuracy [132] */
	s8 = u16;
	/* expect+1: warning: conversion from 'unsigned int' to 'signed char' may lose accuracy [132] */
	s8 = u32;
	/* expect+1: warning: conversion from 'unsigned long long' to 'signed char' may lose accuracy [132] */
	s8 = u64;

	s16 = u8;
	/* expect+1: warning: conversion from 'unsigned int' to 'short' may lose accuracy [132] */
	s16 = u32;
	/* expect+1: warning: conversion from 'unsigned long long' to 'short' may lose accuracy [132] */
	s16 = u64;

	s32 = u8;
	s32 = u16;
	/* expect+1: warning: conversion from 'unsigned long long' to 'int' may lose accuracy [132] */
	s32 = u64;

	s64 = u8;
	s64 = u16;
	s64 = u32;
}

void
signed_to_unsigned(void)
{
	/* expect+1: warning: conversion from 'short' to 'unsigned char' may lose accuracy [132] */
	u8 = s16;
	/* expect+1: warning: conversion from 'int' to 'unsigned char' may lose accuracy [132] */
	u8 = s32;
	/* expect+1: warning: conversion from 'long long' to 'unsigned char' may lose accuracy [132] */
	u8 = s64;

	u16 = s8;
	/* expect+1: warning: conversion from 'int' to 'unsigned short' may lose accuracy [132] */
	u16 = s32;
	/* expect+1: warning: conversion from 'long long' to 'unsigned short' may lose accuracy [132] */
	u16 = s64;

	u32 = s8;
	u32 = s16;
	/* expect+1: warning: conversion from 'long long' to 'unsigned int' may lose accuracy [132] */
	u32 = s64;

	u64 = s8;
	u64 = s16;
	u64 = s32;
}

void
signed_to_signed(void)
{
	/* expect+1: warning: conversion from 'short' to 'signed char' may lose accuracy [132] */
	s8 = s16;
	/* expect+1: warning: conversion from 'int' to 'signed char' may lose accuracy [132] */
	s8 = s32;
	/* expect+1: warning: conversion from 'long long' to 'signed char' may lose accuracy [132] */
	s8 = s64;

	s16 = s8;
	/* expect+1: warning: conversion from 'int' to 'short' may lose accuracy [132] */
	s16 = s32;
	/* expect+1: warning: conversion from 'long long' to 'short' may lose accuracy [132] */
	s16 = s64;

	s32 = s8;
	s32 = s16;
	/* expect+1: warning: conversion from 'long long' to 'int' may lose accuracy [132] */
	s32 = s64;

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
	if (idx > 0.0)
		/* expect+2: error: operands of '+' have incompatible types 'pointer to const char' and 'double' [107] */
		/* expect+1: error: function 'cover_build_plus_minus' expects to return value [214] */
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

/*
 * PR 36668 notices that lint wrongly complains about the possible loss.
 *
 * The expression 'u8_t << 8' is guaranteed to fit into an 'u16_t', and its
 * lower 8 bits are guaranteed to be clear.  'u16_t | u8_t' is guaranteed to
 * fit into 'u16_t'.
 *
 * Since tree.c 1.444 from 2022-05-26, lint tracks simple bitwise and
 * arithmetic constraints across a single expression.
 */
void
be16dec(void)
{
	/*
	 * Before tree.c 1.444 from 2022-05-26, lint complained that the
	 * conversion from 'int' to 'unsigned short' may lose accuracy.
	 */
	u16 = (u16_t)u8 << 8 | u8;
}

/*
 * Since tree.c 1.434 from 2022-04-19, lint infers the possible values of
 * expressions of the form 'integer & constant', see can_represent.
 */
void
be32enc(void)
{
	u8 = u32 >> 24 & 0xff;
	u8 = u32 >> 16 & 0xff;
	u8 = u32 >> 8 & 0xff;
	u8 = u32 & 0xff;
}

void
test_ic_mult(void)
{
	/* expect+1: warning: conversion from 'int' to 'unsigned char' may lose accuracy [132] */
	u8 = u8 * u8;
	u16 = u8 * u8;
	/* expect+1: warning: conversion from 'int' to 'unsigned short' may lose accuracy [132] */
	u16 = u16 * u8;
	u32 = u16 * u16;

	u32 = u16 * 65537ULL;
	/* expect+1: warning: conversion from 'unsigned long long' to 'unsigned int' may lose accuracy [132] */
	u32 = u16 * 65538ULL;

	u16 = 0 * u16;
	u16 = 1 * u16;
	/* expect+1: warning: conversion from 'int' to 'unsigned short' may lose accuracy [132] */
	u16 = 2 * u16;

	// from __BITS, __SHIFTIN, __SHIFTOUT
	u32 = (u16 & 1023ULL) / 1ULL * 1024ULL | (u16 & 1023ULL) / 1ULL * 1ULL;
}

void
test_ic_div(void)
{
	// FIXME
	/* expect+1: warning: conversion from 'int' to 'unsigned char' may lose accuracy [132] */
	u8 = u8 / u8;
	// FIXME
	/* expect+1: warning: conversion from 'int' to 'unsigned char' may lose accuracy [132] */
	u8 = u16 / u8;
	// FIXME
	/* expect+1: warning: conversion from 'int' to 'unsigned short' may lose accuracy [132] */
	u16 = u8 / u8;
	u16 = u32 / 65536;
	/* expect+1: warning: conversion from 'unsigned int' to 'unsigned short' may lose accuracy [132] */
	u16 = u32 / 65535;
}

void
test_ic_mod(void)
{
	/* The result is between 0 and 254. */
	u8 = u64 % u8;

	/* The result is between 0 and 255. */
	u8 = u64 % 256;

	/* The result is between 0 and 256. */
	/* expect+1: warning: conversion from 'unsigned long long' to 'unsigned char' may lose accuracy [132] */
	u8 = u64 % 257;

	/* The result is between 0 and 1000. */
	/* expect+1: warning: conversion from 'unsigned long long' to 'unsigned char' may lose accuracy [132] */
	u8 = u64 % 1000;
	/* expect+1: warning: conversion from 'unsigned long long' to 'unsigned int:9' may lose accuracy [132] */
	bits.u9 = u64 % 1000;
	bits.u10 = u64 % 1000;
	u16 = u64 % 1000;

	s8 = s16 % s8;
	/* expect+1: warning: conversion from 'int' to 'signed char' may lose accuracy [132] */
	s8 = s16 % s16;
	s8 = s64 % 1;
	s8 = s64 % (s16 & 1);
	/* expect+1: warning: conversion from 'long long' to 'signed char' may lose accuracy [132] */
	s8 = s64 % (s16 & 0);
	s8 = (s64 & 0x7f) % s64;
	/* expect+1: warning: conversion from 'long long' to 'signed char' may lose accuracy [132] */
	s8 = (s64 & 0xff) % s64;
}

void
test_ic_shl(void)
{
	u64 = u64 << u64;
	s64 = s64 << s64;

	u16 = u8 << 8;
	/* expect+1: warning: conversion from 'int' to 'unsigned short' may lose accuracy [132] */
	u16 = u8 << 9;
	u32 = u16 << 16;
	// XXX: missing warning as UINT has the same rank as INT, see portable_rank_cmp.
	u32 = u16 << 17;
	/* expect+1: warning: shift amount 56 is greater than bit-size 32 of 'int' [122] */
	u64 = u8 << 56;
	u64 = (u64_t)u8 << 56;
	// XXX: missing warning, as the operand types of '=' are the same, thus no conversion.
	u64 = (u64_t)u8 << 57;
	/* expect+1: warning: shift amount 48 is greater than bit-size 32 of 'int' [122] */
	u64 = u16 << 48;
	u64 = (u64_t)u16 << 48;
	// XXX: missing warning, as the operand types of '=' are the same, thus no conversion.
	u64 = (u64_t)u16 << 49;
	/* expect+1: warning: shift amount 32 equals bit-size of 'unsigned int' [267] */
	u64 = u32 << 32;
	u64 = (u64_t)u32 << 32;
	// XXX: missing warning, as the operand types of '=' are the same, thus no conversion.
	u64 = (u64_t)u32 << 33;
}

void
test_ic_shr(void)
{
	u64 = u64 >> u64;
	s64 = s64 >> s64;

	u32 = u64 >> 32;
	/* expect+1: warning: conversion from 'unsigned long long' to 'unsigned int' may lose accuracy [132] */
	u32 = u64 >> 31;
	u16 = u64 >> 48;
	u16 = u32 >> 16;
	u8 = u64 >> 56;
	u8 = u32 >> 24;
	u8 = u16 >> 8;

	/*
	 * No matter whether the big integer is signed or unsigned, the
	 * result of '&' is guaranteed to be an unsigned value.
	 */
	u8 = (s64 & 0xf0) >> 4;
	u8 = (s8 & 0xf0) >> 4;
}

void
test_ic_bitand(void)
{
	u8 = u8 & u16;

	/* expect+1: warning: conversion from 'unsigned int' to 'unsigned char' may lose accuracy [132] */
	u8 = u16 & u32;
}

void
test_ic_bitor(void)
{
	/* expect+1: warning: conversion from 'int' to 'unsigned char' may lose accuracy [132] */
	u8 = u8 | u16;
	u16 = u8 | u16;
	/* expect+1: warning: conversion from 'unsigned int' to 'unsigned short' may lose accuracy [132] */
	u16 = u8 | u32;
	u32 = u8 | u32;
	/* expect+1: warning: conversion from 'unsigned long long' to 'unsigned int' may lose accuracy [132] */
	u32 = u8 | u64;
	u64 = u8 | u64;
}

void
test_ic_quest_colon(char c1, char c2)
{
	/* Both operands are representable as char. */
	ch = cond ? '?' : ':';

	/*
	 * Both operands are representable as char. Clang-Tidy 17 wrongly
	 * warns about a narrowing conversion from 'int' to signed type
	 * 'char'.
	 */
	ch = cond ? c1 : c2;

	/*
	 * Mixing s8 and u8 results in a number from -128 to 255, which neither
	 * fits in s8 nor u8.
	 */
	/* expect+1: warning: conversion from 'int' to 'signed char' may lose accuracy [132] */
	s8 = cond ? s8 : u8;
	/* expect+1: warning: conversion from 'int' to 'unsigned char' may lose accuracy [132] */
	u8 = cond ? s8 : u8;
}

void
test_ic_con(void)
{
	/* expect+1: warning: assignment of negative constant -1 to unsigned type 'unsigned char' [164] */
	u8 = -1;
	u8 = 0;
	u8 = 255;
	/* expect+1: warning: constant truncated by assignment [165] */
	u8 = 256;

	/* expect+1: warning: conversion of 'int' to 'signed char' is out of range [119] */
	s8 = -129;
	s8 = -128;
	s8 = 127;
	/* expect+1: warning: conversion of 'int' to 'signed char' is out of range [119] */
	s8 = 128;
}

void
test_ic_cvt(void)
{
	u16 = (u32 & 0x0000ff00);
	u16 = (u32_t)(u32 & 0x0000ff00);
	u16 = (u16_t)u32;
	u16 = (u8_t)(u32 & 0xffff) << 8;
}

unsigned char
test_bit_fields(unsigned long long m)
{
	/* expect+1: warning: conversion from 'unsigned long long' to 'unsigned int:3' may lose accuracy [132] */
	bits.u3 = bits.u32 & m;

	bits.u5 = bits.u3 & m;
	bits.u32 = bits.u5 & m;

	/* expect+1: warning: conversion from 'unsigned long long' to 'unsigned char' may lose accuracy [132] */
	return bits.u32 & m;
}

void
compare_bit_field_to_integer_constant(void)
{
	static _Bool b;
	static struct {
		short s16:15;
		unsigned short u16:15;
		int s32:15;
		unsigned u32:15;
		long long s64:15;
		unsigned long long u64:15;
	} s;

	// Since decl.c 1.180 from 2021-05-02 and before tree.c 1.624 from
	// 2024-03-12, lint warned about a possible loss of accuracy [132]
	// when promoting an 'unsigned long long' bit-field to 'int'.
	b = s.s16 == 0;
	b = s.u16 == 0;
	b = s.s32 == 0;
	b = s.u32 == 0;
	b = s.s64 == 0;
	b = s.u64 == 0;
	b = !b;
}

/*
 * Before tree.c 1.626 from 2024-03-26, the usual arithmetic conversions for
 * bit-field types with the same base type but different widths simply took
 * the type of the left operand, leading to wrong warnings about loss of
 * accuracy when the right operand was wider than the left operand.
 */
void
binary_operators_on_bit_fields(void)
{
	struct {
		u64_t u15:15;
		u64_t u48:48;
		u64_t u64;
	} s = { 0, 0, 0 };

	u64 = s.u15 | s.u48;
	u64 = s.u48 | s.u15;
	u64 = s.u15 | s.u48 | s.u64;
	u64 = s.u64 | s.u48 | s.u15;
	cond = (s.u15 | s.u48 | s.u64) != 0;
	cond = (s.u64 | s.u48 | s.u15) != 0;

	// Before tree.c from 1.638 from 2024-05-01, lint wrongly warned:
	// warning: conversion of 'int' to 'int:4' is out of range [119]
	s32 = 8 - bits.u3;
}

unsigned char
combine_arithmetic_and_bit_operations(void)
{
	return 0xc0 | (u32 & 0x07c0) / 64;
}
