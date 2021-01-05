/*	$NetBSD: msg_324.c,v 1.3 2021/01/05 23:20:53 rillig Exp $	*/
# 3 "msg_324.c"

// Test for message: suggest cast from '%s' to '%s' on op %s to avoid overflow [324]

/*
 * This warning applies to binary operators if the result of the operator
 * is converted to a type that is bigger than the operands' result type
 * after the usual arithmetic promotions.
 *
 * In such a case, the operator's result would be truncated to the operator's
 * result type (invoking undefined behavior for signed integers), and that
 * truncated value would then be converted.  At that point, a few bits may
 * have been lost.
 */

/* lint1-flags: -g -S -w -P */

void
example(char c, int i, unsigned u)
{
	long l;
	unsigned long ul;

	l = c + i;
	l = i - c;
	ul = c * u;
	ul = u + c;
	ul = i - u;
	ul = u * i;
	l = i << c;

	/*
	 * The operators SHR, DIV and MOD cannot produce an overflow,
	 * therefore no warning is necessary for them.
	 */
	l = i >> c;
	ul = u / c;
	ul = u % c;

	/*
	 * Assigning the result of an increment or decrement operator to a
	 * differently-sized type is no unusual that there is no need to warn
	 * about it.  It's also more unlikely that there is an actual loss
	 * since this only happens for a single value of the old type, unlike
	 * "ul = u * u", which has many more possibilities for overflowing.
	 */
	ul = u++;
	ul = ++u;
	ul = u--;
	ul = --u;
}
