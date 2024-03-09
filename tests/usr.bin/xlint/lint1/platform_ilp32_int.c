/*	$NetBSD: platform_ilp32_int.c,v 1.3 2024/03/09 16:47:09 rillig Exp $	*/
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

char ch;

void
array_index(void)
{
	static char buf[20];

	/* expect+1: warning: array subscript cannot be > 19: 2147483647 [168] */
	ch += buf[2147483647];
	/* expect+2: warning: conversion of 'long long' to 'int' is out of range [119] */
	/* expect+1: warning: array subscript cannot be negative: -2147483648 [167] */
	ch += buf[2147483648];
	/* expect+2: warning: conversion of 'unsigned int' to 'int' is out of range [119] */
	/* expect+1: warning: array subscript cannot be negative: -2147483648 [167] */
	ch += buf[0x80000000];
	/* expect+2: warning: conversion of 'unsigned int' to 'int' is out of range [119] */
	/* expect+1: warning: array subscript cannot be negative: -1 [167] */
	ch += buf[0xffffffff];
	/* expect+1: warning: array subscript cannot be negative: -1 [167] */
	ch += buf[0xffffffffffffffff];
}
