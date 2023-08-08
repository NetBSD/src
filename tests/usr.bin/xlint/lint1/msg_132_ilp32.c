/*	$NetBSD: msg_132_ilp32.c,v 1.4 2023/08/08 19:57:23 rillig Exp $	*/
# 3 "msg_132_ilp32.c"

// Test for message: conversion from '%s' to '%s' may lose accuracy [132]

/*
 * On 32-bit platforms, it is possible to add a 64-bit integer to a 32-bit
 * pointer.  The 64-bit integer is then converted to the ptrdiff_t of the
 * target platform, which results in the non-obvious conversion from
 * 'long long' to either 'long' or 'int', depending on the platform's
 * ptrdiff_t.
 */

/* lint1-only-if: ilp32 int */
/* lint1-extra-flags: -a -X 351 */

/*
 * Seen in usr.bin/make/var.c, function RegexReplace, in the function call
 * SepBuf_AddBytesBetween(buf, wp + m[0].rm_so, wp + m[0].rm_eo).  The
 * offsets of regular expression matches have type off_t, which is a 64-bit
 * integer.
 *
 * C11 6.5.6p8 does not explicitly define the meaning of a pointer + an
 * overly long integer, it just says "undefined behavior" if the resulting
 * pointer would be outside the object.
 */
const char *
array_subscript(const char *p, long long idx)
{
	/* expect+1: warning: conversion from 'long long' to 'int' may lose accuracy [132] */
	return p + idx;
}

/*
 * On ILP32 platforms, pointer, long and int have the same size, so there is
 * no loss of accuracy.
 */
unsigned int
convert_pointer_to_smaller_integer(void *ptr)
{
	return (unsigned long)(ptr) >> 12;
}

_Bool bool_var;
char char_var;
signed char schar_var;
unsigned char uchar_var;
short short_var;
unsigned short ushort_var;
int int_var;
unsigned int uint_var;
long long_var;
unsigned long ulong_var;
long long llong_var;
unsigned long long ullong_var;

void
convert_all(void)
{
	bool_var = bool_var;
	bool_var = char_var;
	bool_var = schar_var;
	bool_var = uchar_var;
	bool_var = short_var;
	bool_var = ushort_var;
	bool_var = int_var;
	bool_var = uint_var;
	bool_var = long_var;
	bool_var = ulong_var;
	bool_var = llong_var;
	bool_var = ullong_var;

	char_var = bool_var;
	char_var = char_var;
	char_var = schar_var;
	char_var = uchar_var;
	char_var = short_var;
	char_var = ushort_var;
	/* expect+1: warning: conversion from 'int' to 'char' may lose accuracy [132] */
	char_var = int_var;
	/* expect+1: warning: conversion from 'unsigned int' to 'char' may lose accuracy [132] */
	char_var = uint_var;
	/* expect+1: warning: conversion from 'long' to 'char' may lose accuracy [132] */
	char_var = long_var;
	/* expect+1: warning: conversion from 'unsigned long' to 'char' may lose accuracy [132] */
	char_var = ulong_var;
	/* expect+1: warning: conversion from 'long long' to 'char' may lose accuracy [132] */
	char_var = llong_var;
	/* expect+1: warning: conversion from 'unsigned long long' to 'char' may lose accuracy [132] */
	char_var = ullong_var;

	schar_var = bool_var;
	schar_var = char_var;
	schar_var = schar_var;
	schar_var = uchar_var;
	schar_var = short_var;
	schar_var = ushort_var;
	/* expect+1: warning: conversion from 'int' to 'signed char' may lose accuracy [132] */
	schar_var = int_var;
	/* expect+1: warning: conversion from 'unsigned int' to 'signed char' may lose accuracy [132] */
	schar_var = uint_var;
	/* expect+1: warning: conversion from 'long' to 'signed char' may lose accuracy [132] */
	schar_var = long_var;
	/* expect+1: warning: conversion from 'unsigned long' to 'signed char' may lose accuracy [132] */
	schar_var = ulong_var;
	/* expect+1: warning: conversion from 'long long' to 'signed char' may lose accuracy [132] */
	schar_var = llong_var;
	/* expect+1: warning: conversion from 'unsigned long long' to 'signed char' may lose accuracy [132] */
	schar_var = ullong_var;

	uchar_var = bool_var;
	uchar_var = char_var;
	uchar_var = schar_var;
	uchar_var = uchar_var;
	uchar_var = short_var;
	uchar_var = ushort_var;
	/* expect+1: warning: conversion from 'int' to 'unsigned char' may lose accuracy [132] */
	uchar_var = int_var;
	/* expect+1: warning: conversion from 'unsigned int' to 'unsigned char' may lose accuracy [132] */
	uchar_var = uint_var;
	/* expect+1: warning: conversion from 'long' to 'unsigned char' may lose accuracy [132] */
	uchar_var = long_var;
	/* expect+1: warning: conversion from 'unsigned long' to 'unsigned char' may lose accuracy [132] */
	uchar_var = ulong_var;
	/* expect+1: warning: conversion from 'long long' to 'unsigned char' may lose accuracy [132] */
	uchar_var = llong_var;
	/* expect+1: warning: conversion from 'unsigned long long' to 'unsigned char' may lose accuracy [132] */
	uchar_var = ullong_var;

	short_var = bool_var;
	short_var = char_var;
	short_var = schar_var;
	short_var = uchar_var;
	short_var = short_var;
	short_var = ushort_var;
	/* expect+1: warning: conversion from 'int' to 'short' may lose accuracy [132] */
	short_var = int_var;
	/* expect+1: warning: conversion from 'unsigned int' to 'short' may lose accuracy [132] */
	short_var = uint_var;
	/* expect+1: warning: conversion from 'long' to 'short' may lose accuracy [132] */
	short_var = long_var;
	/* expect+1: warning: conversion from 'unsigned long' to 'short' may lose accuracy [132] */
	short_var = ulong_var;
	/* expect+1: warning: conversion from 'long long' to 'short' may lose accuracy [132] */
	short_var = llong_var;
	/* expect+1: warning: conversion from 'unsigned long long' to 'short' may lose accuracy [132] */
	short_var = ullong_var;

	ushort_var = bool_var;
	ushort_var = char_var;
	ushort_var = schar_var;
	ushort_var = uchar_var;
	ushort_var = short_var;
	ushort_var = ushort_var;
	/* expect+1: warning: conversion from 'int' to 'unsigned short' may lose accuracy [132] */
	ushort_var = int_var;
	/* expect+1: warning: conversion from 'unsigned int' to 'unsigned short' may lose accuracy [132] */
	ushort_var = uint_var;
	/* expect+1: warning: conversion from 'long' to 'unsigned short' may lose accuracy [132] */
	ushort_var = long_var;
	/* expect+1: warning: conversion from 'unsigned long' to 'unsigned short' may lose accuracy [132] */
	ushort_var = ulong_var;
	/* expect+1: warning: conversion from 'long long' to 'unsigned short' may lose accuracy [132] */
	ushort_var = llong_var;
	/* expect+1: warning: conversion from 'unsigned long long' to 'unsigned short' may lose accuracy [132] */
	ushort_var = ullong_var;

	int_var = bool_var;
	int_var = char_var;
	int_var = schar_var;
	int_var = uchar_var;
	int_var = short_var;
	int_var = ushort_var;
	int_var = int_var;
	int_var = uint_var;
	int_var = long_var;
	int_var = ulong_var;
	/* expect+1: warning: conversion from 'long long' to 'int' may lose accuracy [132] */
	int_var = llong_var;
	/* expect+1: warning: conversion from 'unsigned long long' to 'int' may lose accuracy [132] */
	int_var = ullong_var;

	uint_var = bool_var;
	uint_var = char_var;
	uint_var = schar_var;
	uint_var = uchar_var;
	uint_var = short_var;
	uint_var = ushort_var;
	uint_var = int_var;
	uint_var = uint_var;
	uint_var = long_var;
	uint_var = ulong_var;
	/* expect+1: warning: conversion from 'long long' to 'unsigned int' may lose accuracy [132] */
	uint_var = llong_var;
	/* expect+1: warning: conversion from 'unsigned long long' to 'unsigned int' may lose accuracy [132] */
	uint_var = ullong_var;

	long_var = bool_var;
	long_var = char_var;
	long_var = schar_var;
	long_var = uchar_var;
	long_var = short_var;
	long_var = ushort_var;
	long_var = int_var;
	long_var = uint_var;
	long_var = long_var;
	long_var = ulong_var;
	/* expect+1: warning: conversion from 'long long' to 'long' may lose accuracy [132] */
	long_var = llong_var;
	/* expect+1: warning: conversion from 'unsigned long long' to 'long' may lose accuracy [132] */
	long_var = ullong_var;

	ulong_var = bool_var;
	ulong_var = char_var;
	ulong_var = schar_var;
	ulong_var = uchar_var;
	ulong_var = short_var;
	ulong_var = ushort_var;
	ulong_var = int_var;
	ulong_var = uint_var;
	ulong_var = long_var;
	ulong_var = ulong_var;
	/* expect+1: warning: conversion from 'long long' to 'unsigned long' may lose accuracy [132] */
	ulong_var = llong_var;
	/* expect+1: warning: conversion from 'unsigned long long' to 'unsigned long' may lose accuracy [132] */
	ulong_var = ullong_var;

	llong_var = bool_var;
	llong_var = char_var;
	llong_var = schar_var;
	llong_var = uchar_var;
	llong_var = short_var;
	llong_var = ushort_var;
	llong_var = int_var;
	llong_var = uint_var;
	llong_var = long_var;
	llong_var = ulong_var;
	llong_var = llong_var;
	llong_var = ullong_var;

	ullong_var = bool_var;
	ullong_var = char_var;
	ullong_var = schar_var;
	ullong_var = uchar_var;
	ullong_var = short_var;
	ullong_var = ushort_var;
	ullong_var = int_var;
	ullong_var = uint_var;
	ullong_var = long_var;
	ullong_var = ulong_var;
	ullong_var = llong_var;
	ullong_var = ullong_var;
}
