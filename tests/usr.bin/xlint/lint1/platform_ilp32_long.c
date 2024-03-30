/*	$NetBSD: platform_ilp32_long.c,v 1.9 2024/03/30 16:47:45 rillig Exp $	*/
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
	 * The '-p' option enables checks that apply independently of the
	 * current platform, assuming that 'long' is always wider than 'int'.
	 * This assumption, when applied on its own, leads to wrong warnings
	 * that a 32-bit 'long' may lose accuracy when converted to a 32-bit
	 * 'int'.
	 *
	 * To avoid these, take a look at the actually possible values of the
	 * right-hand side, and if they fit in the left-hand side, don't warn.
	 */
	s32 = sl32;
	sl32 = s32;
	u32 = ul32;
	ul32 = u32;
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
	/* expect+2: warning: conversion of 'long long' to 'long' is out of range [119] */
	/* expect+1: warning: array subscript -2147483648 cannot be negative [167] */
	u8 += u8_buf[2147483648];
	/* expect+2: warning: conversion of 'unsigned int' to 'long' is out of range [119] */
	/* expect+1: warning: array subscript -2147483648 cannot be negative [167] */
	u8 += u8_buf[0x80000000];
	/* expect+2: warning: conversion of 'unsigned int' to 'long' is out of range [119] */
	/* expect+1: warning: array subscript -1 cannot be negative [167] */
	u8 += u8_buf[0xffffffff];
	/* expect+2: warning: conversion of 'unsigned int' to 'long' is out of range [119] */
	/* expect+1: warning: array subscript -2147483648 cannot be negative [167] */
	u8 += u8_buf[0x80000000];
	/* expect+2: warning: conversion of 'unsigned int' to 'long' is out of range [119] */
	/* expect+1: warning: array subscript -1 cannot be negative [167] */
	u8 += u8_buf[0xffffffff];
	/* expect+2: warning: conversion of 'long long' to 'long' is out of range [119] */
	/* expect+1: warning: array subscript -1 cannot be negative [167] */
	u8 += u8_buf[0x00ffffffffffffff];
	/* expect+1: warning: array subscript -1 cannot be negative [167] */
	u8 += u8_buf[0xffffffffffffffff];

	/* expect+1: warning: array subscript 16777215 cannot be > 19 [168] */
	u64 += u64_buf[0x00ffffff];
	/* expect+2: warning: '2147483647 * 8' overflows 'long' [141] */
	/* expect+1: warning: array subscript 268435455 cannot be > 19 [168] */
	u64 += u64_buf[0x7fffffff];
	/* expect+3: warning: conversion of 'long long' to 'long' is out of range [119] */
	/* expect+2: warning: '-2147483648 * 8' overflows 'long' [141] */
	/* expect+1: warning: array subscript -268435456 cannot be negative [167] */
	u64 += u64_buf[2147483648];
	/* expect+3: warning: conversion of 'unsigned int' to 'long' is out of range [119] */
	/* expect+2: warning: '-2147483648 * 8' overflows 'long' [141] */
	/* expect+1: warning: array subscript -268435456 cannot be negative [167] */
	u64 += u64_buf[0x80000000];
	/* expect+2: warning: conversion of 'unsigned int' to 'long' is out of range [119] */
	/* expect+1: warning: array subscript -1 cannot be negative [167] */
	u64 += u64_buf[0xffffffff];
	/* expect+3: warning: conversion of 'unsigned int' to 'long' is out of range [119] */
	/* expect+2: warning: '-2147483648 * 8' overflows 'long' [141] */
	/* expect+1: warning: array subscript -268435456 cannot be negative [167] */
	u64 += u64_buf[0x80000000];
	/* expect+2: warning: conversion of 'unsigned int' to 'long' is out of range [119] */
	/* expect+1: warning: array subscript -1 cannot be negative [167] */
	u64 += u64_buf[0xffffffff];
	/* expect+2: warning: conversion of 'long long' to 'long' is out of range [119] */
	/* expect+1: warning: array subscript -1 cannot be negative [167] */
	u64 += u64_buf[0x00ffffffffffffff];
	/* expect+2: warning: conversion of 'long long' to 'long' is out of range [119] */
	/* expect+1: warning: array subscript -1 cannot be negative [167] */
	u64 += u64_buf[0x0fffffffffffffff];
	/* expect+2: warning: conversion of 'long long' to 'long' is out of range [119] */
	/* expect+1: warning: array subscript -1 cannot be negative [167] */
	u64 += u64_buf[0x1fffffffffffffff];
	/* expect+2: warning: conversion of 'long long' to 'long' is out of range [119] */
	/* expect+1: warning: array subscript -1 cannot be negative [167] */
	u64 += u64_buf[0x3fffffffffffffff];
	/* expect+2: warning: conversion of 'long long' to 'long' is out of range [119] */
	/* expect+1: warning: array subscript -1 cannot be negative [167] */
	u64 += u64_buf[0x7fffffffffffffff];
	/* expect+1: warning: array subscript -1 cannot be negative [167] */
	u64 += u64_buf[0xffffffffffffffff];
}
