/*	$NetBSD: msg_309.c,v 1.5 2022/06/17 06:59:16 rillig Exp $	*/
# 3 "msg_309.c"

// Test for message: extra bits set to 0 in conversion of '%s' to '%s', op '%s' [309]

int
scale(unsigned long long x) {

	/*
	 * Both operands of '&' have the same type, therefore no conversion
	 * is necessary and no bits can get lost.
	 */
	if ((x & 0xffffffff00000000ULL) != 0)
		return 32;

	/*
	 * The constant has type 'unsigned 32-bit'.  The usual arithmetic
	 * conversions of '&' convert this constant to unsigned 64-bit.
	 * The programmer may or may not have intended to sign-extend the
	 * bit mask here.  This situation may occur during migration from a
	 * 32-bit to a 64-bit platform.
	 */
	/* expect+1: warning: extra bits set to 0 in conversion of 'unsigned int' to 'unsigned long long', op '&' [309] */
	if ((x & 0xffff0000) != 0)
		return 16;

	/*
	 * In the remaining cases, the constant does not have its most
	 * significant bit set, therefore there is no ambiguity.
	 */
	if ((x & 0xff00) != 0)
		return 8;
	if ((x & 0xf0) != 0)
		return 4;
	if ((x & 0xc) != 0)
		return 2;
	if ((x & 0x2) != 0)
		return 1;
	return (int)(x & 0x1);
}
