/*	$NetBSD: msg_382.c,v 1.1 2024/07/10 20:33:38 rillig Exp $	*/
# 3 "msg_382.c"

// Test for message: constant assignment of type '%s' in operand of '!' always evaluates to '%s' [382]

/*
 * Outside strict bool mode, an assignment can be used as a condition, but
 * that is generally wrong.  Especially if a constant is assigned to a
 * variable, the condition always evaluates to that constant value, which
 * indicates a typo, as '==' makes more sense than '=' in a condition.
 */

/* lint1-extra-flags: -X 351 */

int
/* expect+1: warning: parameter 'b' unused in function 'conversions' [231] */
conversions(int a, int b)
{
	/* expect+1: warning: constant assignment of type 'int' in operand of '!' always evaluates to 'true' [382] */
	if (!(a = 13))
		return 1;
	/* expect+1: warning: constant assignment of type 'int' in operand of '!' always evaluates to 'false' [382] */
	if (!(b = 0))
		return 2;
	if (!(a = a + 1))
		return 3;
	return a;
}
