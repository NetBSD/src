/*	$NetBSD: msg_117.c,v 1.2 2021/01/09 14:37:16 rillig Exp $	*/
# 3 "msg_117.c"

// Test for message: bitwise operation on signed value possibly nonportable [117]

/* lint1-extra-flags: -p */

int
shr(int a, int b)
{
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
	return -0x1234 >> a;
}

int
shr_rhs_constant_positive(int a)
{
	return a >> 0x1234;
}

int
shr_rhs_constant_negative(int a)
{
	return a >> -0x1234;
}
