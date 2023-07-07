/*	$NetBSD: msg_324.c,v 1.9 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_324.c"

// Test for message: suggest cast from '%s' to '%s' on op '%s' to avoid overflow [324]

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

/* lint1-flags: -g -S -w -P -X 351 */

void
example(char c, int i, unsigned u)
{
	long long ll;
	unsigned long long ull;

	/* expect+2: warning: 'll' set but not used in function 'example' [191] */
	/* expect+1: warning: suggest cast from 'int' to 'long long' on op '+' to avoid overflow [324] */
	ll = c + i;
	/* expect+1: warning: suggest cast from 'int' to 'long long' on op '-' to avoid overflow [324] */
	ll = i - c;
	/* expect+2: warning: 'ull' set but not used in function 'example' [191] */
	/* expect+1: warning: suggest cast from 'unsigned int' to 'unsigned long long' on op '*' to avoid overflow [324] */
	ull = c * u;
	/* expect+1: warning: suggest cast from 'unsigned int' to 'unsigned long long' on op '+' to avoid overflow [324] */
	ull = u + c;
	/* expect+1: warning: suggest cast from 'unsigned int' to 'unsigned long long' on op '-' to avoid overflow [324] */
	ull = i - u;
	/* expect+1: warning: suggest cast from 'unsigned int' to 'unsigned long long' on op '*' to avoid overflow [324] */
	ull = u * i;
	/* expect+1: warning: suggest cast from 'int' to 'long long' on op '<<' to avoid overflow [324] */
	ll = i << c;

	/*
	 * The operators SHR, DIV and MOD cannot produce an overflow,
	 * therefore no warning is necessary for them.
	 */
	ll = i >> c;
	ull = u / c;
	ull = u % c;

	/*
	 * Assigning the result of an increment or decrement operator to a
	 * differently-sized type is no unusual that there is no need to warn
	 * about it.  It's also more unlikely that there is an actual loss
	 * since this only happens for a single value of the old type, unlike
	 * "ull = u * u", which has many more possibilities for overflowing.
	 */
	ull = u++;
	ull = ++u;
	ull = u--;
	ull = --u;
}
