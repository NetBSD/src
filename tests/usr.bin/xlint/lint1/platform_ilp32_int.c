/*	$NetBSD: platform_ilp32_int.c,v 1.8 2024/03/30 17:12:26 rillig Exp $	*/
# 3 "platform_ilp32_int.c"

/*
 * Test features that only apply to platforms that have 32-bit int, long and
 * pointer types and where size_t is unsigned int, not unsigned long.
 */

/* lint1-only-if: ilp32 int */
/* lint1-extra-flags: -c -h -a -p -b -r -z -X 351 */

int s32;
unsigned int u32;
long sl32;
unsigned long ul32;

void
convert_between_int_and_long(void)
{
	/*
	 * No warning about possible loss of accuracy, as the types have the
	 * same size, both in target platform mode as well as in portable
	 * mode.
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
	/* expect+2: warning: conversion of 'long long' to 'int' is out of range [119] */
	/* expect+1: warning: array subscript -2147483648 cannot be negative [167] */
	u8 += u8_buf[2147483648];
	/* expect+1: warning: array subscript 2147483648 cannot be > 19 [168] */
	u8 += u8_buf[0x80000000];
	/* expect+1: warning: array subscript 4294967295 cannot be > 19 [168] */
	u8 += u8_buf[0xffffffff];
	/* expect+1: warning: array subscript 2147483648 cannot be > 19 [168] */
	u8 += u8_buf[0x80000000];
	/* expect+1: warning: array subscript 4294967295 cannot be > 19 [168] */
	u8 += u8_buf[0xffffffff];
	/* expect+2: warning: conversion of 'long long' to 'int' is out of range [119] */
	/* expect+1: warning: array subscript -1 cannot be negative [167] */
	u8 += u8_buf[0x00ffffffffffffff];
	/* expect+1: warning: array subscript -1 cannot be negative [167] */
	u8 += u8_buf[0xffffffffffffffff];

	/* expect+1: warning: array subscript 16777215 cannot be > 19 [168] */
	u64 += u64_buf[0x00ffffff];
	/* expect+2: warning: '2147483647 * 8' overflows 'int' [141] */
	/* expect+1: warning: array subscript 268435455 cannot be > 19 [168] */
	u64 += u64_buf[0x7fffffff];
	/* expect+3: warning: conversion of 'long long' to 'int' is out of range [119] */
	/* expect+2: warning: '-2147483648 * 8' overflows 'int' [141] */
	/* expect+1: warning: array subscript -268435456 cannot be negative [167] */
	u64 += u64_buf[2147483648];
	/* expect+1: warning: '2147483648 * 8' overflows 'unsigned int' [141] */
	u64 += u64_buf[0x80000000];
	/* expect+2: warning: '4294967295 * 8' overflows 'unsigned int' [141] */
	/* expect+1: warning: array subscript 536870911 cannot be > 19 [168] */
	u64 += u64_buf[0xffffffff];
	/* expect+1: warning: '2147483648 * 8' overflows 'unsigned int' [141] */
	u64 += u64_buf[0x80000000];
	/* expect+2: warning: '4294967295 * 8' overflows 'unsigned int' [141] */
	/* expect+1: warning: array subscript 536870911 cannot be > 19 [168] */
	u64 += u64_buf[0xffffffff];
	/* expect+2: warning: conversion of 'long long' to 'int' is out of range [119] */
	/* expect+1: warning: array subscript -1 cannot be negative [167] */
	u64 += u64_buf[0x00ffffffffffffff];
	/* expect+2: warning: conversion of 'long long' to 'int' is out of range [119] */
	/* expect+1: warning: array subscript -1 cannot be negative [167] */
	u64 += u64_buf[0x0fffffffffffffff];
	/* expect+2: warning: conversion of 'long long' to 'int' is out of range [119] */
	/* expect+1: warning: array subscript -1 cannot be negative [167] */
	u64 += u64_buf[0x1fffffffffffffff];
	/* expect+2: warning: conversion of 'long long' to 'int' is out of range [119] */
	/* expect+1: warning: array subscript -1 cannot be negative [167] */
	u64 += u64_buf[0x3fffffffffffffff];
	/* expect+2: warning: conversion of 'long long' to 'int' is out of range [119] */
	/* expect+1: warning: array subscript -1 cannot be negative [167] */
	u64 += u64_buf[0x7fffffffffffffff];
	/* expect+1: warning: array subscript -1 cannot be negative [167] */
	u64 += u64_buf[0xffffffffffffffff];
}
