/*	$NetBSD: msg_379.c,v 1.1 2024/05/11 15:53:38 rillig Exp $	*/
# 3 "msg_379.c"

// Test for message: comparing integer '%s' to floating point constant %Lg [379]

/*
 * Comparing an integer expression to a floating point constant mixes
 * different kinds of types.  This mixture is more complicated than necessary,
 * thus confusing human readers.
 *
 * The compilers are fine with this kind of expression: GCC treats the
 * constant as an integer even at -O0 while Clang needs at least -O.
 */

/* lint1-extra-flags: -X 351 */

int
comparisons(int x)
{
	if (3 > 123.0)
		/* expect+1: warning: statement not reached [193] */
		return 0;
	/* expect+1: warning: comparing integer 'int' to floating point constant 123 [379] */
	if (x > 123.0)
		return 1;

	// Yoda-style comparisons are unusual enough to not warn about them.
	if (123.0 > x)
		return 2;
	return 3;
}
