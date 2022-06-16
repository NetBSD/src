/*	$NetBSD: msg_120.c,v 1.6 2022/06/16 16:58:36 rillig Exp $	*/
# 3 "msg_120.c"

// Test for message: bitwise '%s' on signed value nonportable [120]

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
