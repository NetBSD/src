/*	$NetBSD: msg_132_ilp32.c,v 1.2 2022/06/10 18:29:01 rillig Exp $	*/
# 3 "msg_132_ilp32.c"

// Test for message: conversion from '%s' to '%s' may lose accuracy [132]

/*
 * On 32-bit platforms, it is possible to add a 64-bit integer to a 32-bit
 * pointer.  The 64-bit integer is then converted to the ptrdiff_t of the
 * target platform, which results in the non-obvious conversion from
 * 'long long' to either 'long' or 'int', depending on the platform's
 * ptrdiff_t.
 */

/* lint1-extra-flags: -a */
/* lint1-only-if: ilp32 int */

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
