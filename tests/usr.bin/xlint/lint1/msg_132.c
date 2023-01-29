/*	$NetBSD: msg_132.c,v 1.25 2023/01/29 17:02:09 rillig Exp $	*/
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

typedef unsigned char u8_t;
typedef unsigned short u16_t;
typedef unsigned int u32_t;
typedef unsigned long long u64_t;
typedef signed char s8_t;
typedef signed short s16_t;
typedef signed int s32_t;
typedef signed long long s64_t;


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
	/* expect+3: error: operands of '+' have incompatible types 'pointer' and 'double' [107] */
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
static inline u16_t
be16dec(const void *buf)
{
	const u8_t *p = buf;

	/*
	 * Before tree.c 1.444 from 2022-05-26, lint complained that the
	 * conversion from 'int' to 'unsigned short' may lose accuracy.
	 */
	return ((u16_t)p[0]) << 8 | p[1];
}

/*
 * Since tree.c 1.434 from 2022-04-19, lint infers the possible values of
 * expressions of the form 'integer & constant', see can_represent.
 */
static inline void
be32enc(void *buf, u32_t u)
{
	u8_t *p = buf;

	p[0] = u >> 24 & 0xff;
	p[1] = u >> 16 & 0xff;
	p[2] = u >> 8 & 0xff;
	p[3] = u & 0xff;
}

u32_t
test_ic_shr(u64_t x)
{
	if (x > 3)
		return x >> 32;
	if (x > 2)
		/* expect+1: warning: conversion from 'unsigned long long' to 'unsigned int' may lose accuracy [132] */
		return x >> 31;

	/* expect+1: warning: conversion from 'unsigned long long' to 'unsigned int' may lose accuracy [132] */
	u32 = u64 >> 31;
	u32 = u64 >> 32;
	u16 = u64 >> 48;
	u8 = u64 >> 56;
	u16 = u32 >> 16;
	u8 = u32 >> 24;
	u8 = u16 >> 8;

	/*
	 * No matter whether the big integer is signed or unsigned, the
	 * result of '&' is guaranteed to be an unsigned value.
	 */
	u8 = (s64 & 0xf0) >> 4;
	u8 = (s8 & 0xf0) >> 4;

	/* expect+1: warning: conversion from 'unsigned long long' to 'unsigned int' may lose accuracy [132] */
	return x;
}

unsigned char
test_bit_fields(unsigned long long m)
{
	/* expect+1: warning: conversion from 'unsigned long long:32' to 'unsigned int:3' may lose accuracy [132] */
	bits.u3 = bits.u32 & m;

	bits.u5 = bits.u3 & m;
	bits.u32 = bits.u5 & m;

	/* expect+1: warning: conversion from 'unsigned long long:32' to 'unsigned char' may lose accuracy [132] */
	return bits.u32 & m;
}

/*
 * Traditional C has an extra rule that the right-hand operand of a bit shift
 * operator is converted to 'int'.  Before tree.c 1.467 from 2022-07-02, this
 * conversion was implemented as a CVT node, which means a cast, not an
 * implicit conversion.  Changing the CVT to NOOP would have caused a wrong
 * warning 'may lose accuracy' in language levels other than traditional C.
 */

u64_t
u64_shl(u64_t lhs, u64_t rhs)
{
	return lhs << rhs;
}

u64_t
u64_shr(u64_t lhs, u64_t rhs)
{
	return lhs >> rhs;
}

s64_t
s64_shl(s64_t lhs, s64_t rhs)
{
	return lhs << rhs;
}

s64_t
s64_shr(s64_t lhs, s64_t rhs)
{
	return lhs >> rhs;
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

	/*
	 * For signed division, if the result of 'a / b' is not representable
	 * exactly, the result of 'a % b' is defined such that
	 * '(a / b) * a + a % b == a'.
	 *
	 * If the result of 'a / b' is not representable exactly, the result
	 * of 'a % b' is not defined.  Due to this uncertainty, lint does not
	 * narrow down the range for signed modulo expressions.
	 *
	 * C90 6.3.5, C99 6.5.5.
	 */

	/* expect+1: warning: conversion from 'int' to 'signed char' may lose accuracy [132] */
	s8 = s16 % s8;

	/*
	 * The result is always 0, it's a theoretical edge case though, so
	 * lint doesn't care to implement this.
	 */
	/* expect+1: warning: conversion from 'long long' to 'signed char' may lose accuracy [132] */
	s8 = s64 % 1;
}

void
test_ic_bitand(void)
{
	/*
	 * ic_bitand assumes that integers are represented in 2's complement,
	 * and that the sign bit of signed integers behaves like a value bit.
	 * That way, the following expressions get their constraints computed
	 * correctly, regardless of whether ic_expr takes care of integer
	 * promotions or not.  Compare ic_mod, which ignores signed types.
	 */

	u8 = u8 & u16;

	/* expect+1: warning: conversion from 'unsigned int' to 'unsigned char' may lose accuracy [132] */
	u8 = u16 & u32;
}
