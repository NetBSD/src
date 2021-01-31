/*	$NetBSD: msg_120.c,v 1.4 2021/01/31 11:44:48 rillig Exp $	*/
# 3 "msg_120.c"

// Test for message: bitwise '%s' on signed value nonportable [120]

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
	return a >> 0x1234;		/* expect: 117, 122 */
}

int
shr_rhs_constant_negative(int a)
{
	return a >> -0x1234;		/* expect: 117, 121 */
}
