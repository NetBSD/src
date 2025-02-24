/*	$NetBSD: msg_309.c,v 1.8 2025/02/24 19:49:00 rillig Exp $	*/
# 3 "msg_309.c"

// Test for message: extra bits set to 0 in conversion of '%s' to '%s', op '%s' [309]

/* lint1-extra-flags: -X 351 */

typedef unsigned char u8_t;
typedef unsigned int u32_t;
typedef unsigned long long u64_t;

u8_t u8;
u32_t u32;
u64_t u64;


void
test(void)
{

	/*
	 * Both operands of '&' have the same type, therefore no conversion
	 * is necessary and no bits can get lost.
	 */
	u64 = u64 & 0xffffffff00000000ULL;

	/*
	 * The constant has type 'unsigned 32-bit'.  The usual arithmetic
	 * conversions of '&' convert this constant to unsigned 64-bit.
	 * The programmer may or may not have intended to sign-extend the
	 * bit mask here.  This situation may occur during migration from a
	 * 32-bit to a 64-bit platform.
	 */
	/* expect+1: warning: extra bits set to 0 in conversion of 'unsigned int' to 'unsigned long long', op '&' [309] */
	u64 = u64 & 0xffff0000;

	/*
	 * The integer constant is explicitly unsigned.  Even in this case,
	 * the code may have originated on a platform where 'x' had 32 bits
	 * originally, and the intention may have been to clear the lower 16
	 * bits.
	 */
	/* expect+1: warning: extra bits set to 0 in conversion of 'unsigned int' to 'unsigned long long', op '&' [309] */
	u64 = u64 & 0xffff0000U;

	/*
	 * Even if the expression is written as '& ~', which makes the
	 * intention of clearing the lower 16 bits clear, on a 32-bit
	 * platform the integer constant stays at 32 bits, and when porting
	 * the code to a 64-bit platform, the upper 32 bits are preserved.
	 */
	/* expect+1: warning: extra bits set to 0 in conversion of 'unsigned int' to 'unsigned long long', op '&' [309] */
	u64 = u64 & ~0xffffU;

	/*
	 * Casting the integer constant to the proper type removes all
	 * ambiguities about the programmer's intention.
	 */
	u64 = u64 & (unsigned long long)~0xffffU;

	/*
	 * In the remaining cases, the constant does not have its most
	 * significant bit set, therefore there is no ambiguity.
	 */
	u64 = u64 & 0xff00;
	u64 = u64 & 0xf0;
	u64 = u64 & 0xc;
	u64 = u64 & 0x2;
	u64 = u64 & 0x1;

	u8 = u8 & 0x7f;
	u8 = u8 & 0x80;
	u8 = u8 & -0x80;
	/* expect+1: warning: extra bits set to 0 in conversion of 'unsigned char' to 'int', op '&' [309] */
	u8 = u8 & (u8_t)-0x80;
	/* expect+1: warning: extra bits set to 0 in conversion of 'unsigned char' to 'int', op '&' [309] */
	u8 = u8 & (u8_t)-0x80U;
}
