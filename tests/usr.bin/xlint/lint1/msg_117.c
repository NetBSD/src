/*	$NetBSD: msg_117.c,v 1.7 2021/08/15 13:08:20 rillig Exp $	*/
# 3 "msg_117.c"

// Test for message: bitwise '%s' on signed value possibly nonportable [117]

/* lint1-extra-flags: -p */

int
shr(int a, int b)
{
	return a >> b;			/* expect: 117 */
}

int
shr_lhs_constant_positive(int a)
{
	return 0x1234 >> a;
}

int
shr_lhs_constant_negative(int a)
{
	return -0x1234 >> a;		/* expect: 120 */
}

int
shr_rhs_constant_positive(int a)
{
	return a >> 0x1234;		/* expect: 117 *//* expect: 122 */
}

int
shr_rhs_constant_negative(int a)
{
	return a >> -0x1234;		/* expect: 117 *//* expect: 121 */
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
