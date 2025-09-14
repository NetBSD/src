/*	$NetBSD: msg_120.c,v 1.8 2025/09/14 11:14:00 rillig Exp $	*/
# 3 "msg_120.c"

// Test for message: bitwise '%s' on signed '%s' nonportable [120]

/* lint1-extra-flags: -p -X 351 */

int
shr(int a, int b)
{
	/* expect+1: warning: bitwise '>>' on signed 'int' possibly nonportable [117] */
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
	/* expect+1: warning: bitwise '>>' on signed 'int' nonportable [120] */
	return -0x1234 >> a;
}

int
shr_rhs_constant_positive(int a)
{
	/* expect+2: warning: bitwise '>>' on signed 'int' possibly nonportable [117] */
	/* expect+1: warning: shift amount 4660 is greater than bit-size 32 of 'int' [122] */
	return a >> 0x1234;
}

int
shr_rhs_constant_negative(int a)
{
	/* expect+2: warning: bitwise '>>' on signed 'int' possibly nonportable [117] */
	/* expect+1: warning: negative shift [121] */
	return a >> -0x1234;
}
