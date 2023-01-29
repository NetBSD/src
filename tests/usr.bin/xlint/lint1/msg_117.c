/*	$NetBSD: msg_117.c,v 1.13 2023/01/29 17:13:10 rillig Exp $	*/
# 3 "msg_117.c"

// Test for message: bitwise '%s' on signed value possibly nonportable [117]

/* lint1-extra-flags: -p */

int
shr(int a, int b)
{
	/* expect+1: warning: bitwise '>>' on signed value possibly nonportable [117] */
	return a >> b;
}

int
shr_lhs_constant_positive(int a)
{
	return 0x1234 >> a;
}

int
shr_lhs_constant_negative(int a)
{
	/* expect+1: warning: bitwise '>>' on signed value nonportable [120] */
	return -0x1234 >> a;
}

int
shr_rhs_constant_positive(int a)
{
	/* expect+2: warning: bitwise '>>' on signed value possibly nonportable [117] */
	/* expect+1: warning: shift amount 4660 is greater than bit-size 32 of 'int' [122] */
	return a >> 0x1234;
}

int
shr_rhs_constant_negative(int a)
{
	/* expect+2: warning: bitwise '>>' on signed value possibly nonportable [117] */
	/* expect+1: warning: negative shift [121] */
	return a >> -0x1234;
}

unsigned int
shr_unsigned_char(unsigned char uc)
{
	/*
	 * Even though 'uc' is promoted to 'int', it cannot be negative.
	 * Before tree.c 1.335 from 2021-08-15, lint wrongly warned that
	 * 'uc >> 4' might be a bitwise '>>' on signed value.
	 */
	return uc >> 4;
}

unsigned char
shr_unsigned_char_promoted_signed(unsigned char bit)
{
	/*
	 * The possible values for 'bit' range from 0 to 255.  Subtracting 1
	 * from 0 results in a negative expression value.
	 */
	/* expect+1: warning: bitwise '>>' on signed value possibly nonportable [117] */
	return (unsigned char)((bit - 1) >> 5);
}

unsigned char
shr_unsigned_char_promoted_unsigned(unsigned char bit)
{
	/*
	 * To prevent the above warning, the intermediate expression must be
	 * cast to 'unsigned char'.
	 */
	return (unsigned char)((unsigned char)(bit - 1) >> 5);
}

/*
 * C90 3.3.7, C99 6.5.7 and C11 6.5.7 all say the same: If E1 has a signed
 * type and a negative value, the resulting value is implementation-defined.
 *
 * These standards don't guarantee anything about the lower bits of the
 * resulting value, which are generally independent of whether the shift is
 * performed in signed arithmetics or in unsigned arithmetics.  The C99
 * rationale talks about signed shifts, but does not provide any guarantee
 * either.  It merely suggests that platforms are free to use unsigned shifts
 * even if the operand type is signed.
 *
 * K&R provides more guarantees by saying: Right shifting a signed quantity
 * will fill with sign bits ("arithmetic shift") on some machines such as the
 * PDP-11, and with 0-bits ("logical shift") on others.
 *
 * https://gcc.gnu.org/onlinedocs/gcc/Integers-implementation.html says:
 * Signed '>>' acts on negative numbers by sign extension.
 *
 * This means that at least in GCC mode, lint may decide to not warn about
 * these cases.
 */
void
shr_signed_ignoring_high_bits(int x)
{

	/*
	 * All sane platforms should define that 'x >> 0 == x', even if x is
	 * negative.
	 */
	/* expect+1: warning: bitwise '>>' on signed value possibly nonportable [117] */
	if (x >> 0 != 0)
		return;

	/*
	 * If x is negative, x >> 1 is nonzero, no matter whether the shift
	 * is arithmetic or logical.
	 */
	/* expect+1: warning: bitwise '>>' on signed value possibly nonportable [117] */
	if (x >> 1 != 0)
		return;

	/*
	 * The highest bit may be 0 or 1, the others should be well-defined
	 * on all sane platforms, making it irrelevant whether the actual
	 * shift operation is arithmetic or logical.
	 */
	/* expect+1: warning: bitwise '>>' on signed value possibly nonportable [117] */
	if (((x >> 1) & 1) != 0)
		return;

	/*
	 * The result of this expression is the same with arithmetic and
	 * logical shifts since the filled bits are masked out.
	 */
	/* expect+1: warning: bitwise '>>' on signed value possibly nonportable [117] */
	if (((x >> 31) & 1) != 0)
		return;

	/*
	 * In this case, arithmetic shift results in 2 while logical shift
	 * results in 0.  This difference is what this warning is about.
	 */
	/* expect+1: warning: bitwise '>>' on signed value possibly nonportable [117] */
	if (((x >> 31) & 2) != 0)
		return;

	/*
	 * The result of '&' is guaranteed to be positive, so don't warn.
	 * Code like this typically occurs in hexdump functions.
	 */
	if ((x & 0xf0) >> 4 != 0)
		return;
}
