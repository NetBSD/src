/*	$NetBSD: platform_lp64.c,v 1.18 2025/05/15 21:15:31 rillig Exp $	*/
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

unsigned char u8;
unsigned long long u64;
unsigned char u8_buf[20];
unsigned long long u64_buf[20];

void
array_index(void)
{

	/* expect+1: warning: array subscript 16777215 cannot be > 19 [168] */
	u8 += u8_buf[0x00ffffff];
	/* expect+1: warning: array subscript 2147483647 cannot be > 19 [168] */
	u8 += u8_buf[0x7fffffff];
	/* expect+1: warning: array subscript 2147483648 cannot be > 19 [168] */
	u8 += u8_buf[2147483648];
	/* expect+1: warning: array subscript 2147483648 cannot be > 19 [168] */
	u8 += u8_buf[0x80000000];
	/* expect+1: warning: array subscript 4294967295 cannot be > 19 [168] */
	u8 += u8_buf[0xffffffff];
	/* expect+1: warning: array subscript 2147483648 cannot be > 19 [168] */
	u8 += u8_buf[0x80000000];
	/* expect+1: warning: array subscript 4294967295 cannot be > 19 [168] */
	u8 += u8_buf[0xffffffff];
	/* expect+1: warning: array subscript 72057594037927935 cannot be > 19 [168] */
	u8 += u8_buf[0x00ffffffffffffff];
	/* expect+1: warning: array subscript 18446744073709551615 cannot be > 19 [168] */
	u8 += u8_buf[0xffffffffffffffff];

	/* expect+1: warning: array subscript 16777215 cannot be > 19 [168] */
	u64 += u64_buf[0x00ffffff];
	/* expect+1: warning: array subscript 2147483647 cannot be > 19 [168] */
	u64 += u64_buf[0x7fffffff];
	/* expect+1: warning: array subscript 2147483648 cannot be > 19 [168] */
	u64 += u64_buf[2147483648];
	/* expect+1: warning: array subscript 2147483648 cannot be > 19 [168] */
	u64 += u64_buf[0x80000000];
	/* expect+1: warning: array subscript 4294967295 cannot be > 19 [168] */
	u64 += u64_buf[0xffffffff];
	/* expect+1: warning: array subscript 2147483648 cannot be > 19 [168] */
	u64 += u64_buf[0x80000000];
	/* expect+1: warning: array subscript 4294967295 cannot be > 19 [168] */
	u64 += u64_buf[0xffffffff];
	/* expect+1: warning: array subscript 72057594037927935 cannot be > 19 [168] */
	u64 += u64_buf[0x00ffffffffffffff];
	/* expect+1: warning: array subscript 1152921504606846975 cannot be > 19 [168] */
	u64 += u64_buf[0x0fffffffffffffff];
	/* expect+2: warning: '2305843009213693951 * 8' overflows 'long' [141] */
	/* expect+1: warning: array subscript 1152921504606846975 cannot be > 19 [168] */
	u64 += u64_buf[0x1fffffffffffffff];
	/* expect+2: warning: '4611686018427387903 * 8' overflows 'long' [141] */
	/* expect+1: warning: array subscript 1152921504606846975 cannot be > 19 [168] */
	u64 += u64_buf[0x3fffffffffffffff];
	/* expect+2: warning: '9223372036854775807 * 8' overflows 'long' [141] */
	/* expect+1: warning: array subscript 1152921504606846975 cannot be > 19 [168] */
	u64 += u64_buf[0x7fffffffffffffff];
	/* expect+2: warning: '18446744073709551615 * 8' overflows 'unsigned long' [141] */
	/* expect+1: warning: array subscript 2305843009213693951 cannot be > 19 [168] */
	u64 += u64_buf[0xffffffffffffffff];
}

extern const unsigned short *_ctype_tab_;

int
msg_341(void)
{
	// https://mail-index.netbsd.org/current-users/2024/12/15/msg045888.html
	/* expect+1: warning: argument to 'function from <ctype.h>' must be 'unsigned char' or EOF, not 'unsigned int' [341] */
	return (((int)((_ctype_tab_ + 1)[(0xffffffffu)])));

}

void
msg_122(void)
{
	typedef unsigned typedef_type_identifier;
	__attribute__((__mode__(TI))) typedef unsigned attr_typedef_type_identifier;
	typedef __attribute__((__mode__(TI))) unsigned typedef_attr_type_identifier;
	typedef unsigned __attribute__((__mode__(TI))) typedef_type_attr_identifier;
	typedef unsigned typedef_type_identifier_attr __attribute__((__mode__(TI)));
	__attribute__(()) __attribute__((__mode__(TI))) typedef unsigned attr_attr_typedef_type_identifier;
	typedef __attribute__(()) __attribute__((__mode__(TI))) unsigned typedef_attr_attr_type_identifier;
	typedef unsigned __attribute__(()) __attribute__((__mode__(TI))) typedef_type_attr_attr_identifier;
	typedef unsigned typedef_type_identifier_attr_attr __attribute__(()) __attribute__((__mode__(TI)));

	struct {
		typedef_type_identifier typedef_type_identifier;
		attr_typedef_type_identifier attr_typedef_type_identifier;
		typedef_attr_type_identifier typedef_attr_type_identifier;
		typedef_type_attr_identifier typedef_type_attr_identifier;
		typedef_type_identifier_attr typedef_type_identifier_attr;
		attr_attr_typedef_type_identifier attr_attr_typedef_type_identifier;
		typedef_attr_attr_type_identifier typedef_attr_attr_type_identifier;
		typedef_type_attr_attr_identifier typedef_type_attr_attr_identifier;
		typedef_type_identifier_attr_attr typedef_type_identifier_attr_attr;
	} s = {0};

	/* expect+1: warning: shift amount 80 is greater than bit-size 32 of 'unsigned int' [122] */
	u128 = s.typedef_type_identifier << 80;
	u128 = s.attr_typedef_type_identifier << 80;
	u128 = s.typedef_attr_type_identifier << 80;
	u128 = s.typedef_type_attr_identifier << 80;
	// FIXME
	/* expect+1: warning: shift amount 80 is greater than bit-size 32 of 'unsigned int' [122] */
	u128 = s.typedef_type_identifier_attr << 80;
	u128 = s.attr_attr_typedef_type_identifier << 80;
	u128 = s.typedef_attr_attr_type_identifier << 80;
	u128 = s.typedef_type_attr_attr_identifier << 80;
	// FIXME
	/* expect+1: warning: shift amount 80 is greater than bit-size 32 of 'unsigned int' [122] */
	u128 = s.typedef_type_identifier_attr_attr << 80;

	unsigned type_identifier = 0;
	__attribute__((__mode__(TI))) unsigned attr_type_identifier = 0;
	unsigned __attribute__((__mode__(TI))) type_attr_identifier = 0;
	unsigned type_identifier_attr __attribute__((__mode__(TI))) = 0;
	__attribute__(()) __attribute__((__mode__(TI))) unsigned attr_attr_type_identifier = 0;
	unsigned __attribute__(()) __attribute__((__mode__(TI))) type_attr_attr_identifier = 0;
	unsigned type_identifier_attr_attr __attribute__(()) __attribute__((__mode__(TI))) = 0;

	/* expect+1: warning: shift amount 80 is greater than bit-size 32 of 'unsigned int' [122] */
	u128 = type_identifier << 80;
	u128 = attr_type_identifier << 80;
	u128 = type_attr_identifier << 80;
	// FIXME
	/* expect+1: warning: shift amount 80 is greater than bit-size 32 of 'unsigned int' [122] */
	u128 = type_identifier_attr << 80;
	u128 = attr_attr_type_identifier << 80;
	u128 = type_attr_attr_identifier << 80;
	// FIXME
	/* expect+1: warning: shift amount 80 is greater than bit-size 32 of 'unsigned int' [122] */
	u128 = type_identifier_attr_attr << 80;
}
