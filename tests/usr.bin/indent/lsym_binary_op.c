/* $NetBSD: lsym_binary_op.c,v 1.6 2022/04/24 10:36:37 rillig Exp $ */

/*
 * Tests for the token lsym_binary_op, which represents a binary operator in
 * an expression.  Examples for binary operators are '>>', '=', '+', '&&'.
 *
 * Binary operators are surrounded by blanks.
 *
 * Some tokens like '+', '*' or '&' can be either binary or unary operators,
 * with an entirely different meaning.
 *
 * The token '*' is not only a binary or a unary operator, it is used in types
 * as well, to derive a pointer type.
 *
 * See also:
 *	lsym_postfix_op.c	for postfix unary operators
 *	lsym_unary_op.c		for prefix unary operators
 *	lsym_colon.c		for ':'
 *	lsym_question.c		for '?'
 *	lsym_comma.c		for ','
 *	C99 6.4.6		"Punctuators"
 */

//indent input
void
binary_operators(void)
{
	/* In the order of appearance in C11 6.5. */
	a = a * a;
	a = a / a;
	a = a % a;
	a = a + a;
	a = a - a;
	a = a << a;
	a = a >> a;
	a = a < a;
	a = a > a;
	a = a <= a;
	a = a >= a;
	a = a == a;
	a = a != a;
	a = a & a;
	a = a ^ a;
	a = a | a;
	a = a && a;
	a = a || a;
	a = a ? a : a;
	a = a;
	a *= a;
	a /= a;
	a %= a;
	a += a;
	a -= a;
	a <<= a;
	a >>= a;
	a &= a;
	a ^= a;
	a |= a;
	a = a, a;
}
//indent end

//indent run-equals-input


/*
 * If a '*' is immediately followed by another '*', they still form separate
 * operators. The first is a binary operator, the second is unary.
 */
//indent input
int var = expr**ptr;
//indent end

//indent run -di0
int var = expr * *ptr;
//indent end


/*
 * When indent tokenizes some operators, it allows for
 * arbitrary repetitions of the operator character, followed by an
 * arbitrary amount of '='.  This is used for operators like '&&' or
 * '|||==='.
 *
 * Before 2021-03-07 22:11:01, the comment '//' was treated as an
 * operator as well, and so was the comment '/////', leading to
 * unexpected results.
 *
 * See lexi.c, lexi, "default:".
 */
//indent input
void
long_run_of_operators(void)
{
	if (a &&&&&&& b)
		return;
	if (a |||=== b)
		return;
}
//indent end

//indent run-equals-input


/*
 * Long chains of '+' and '-' must be split into several operators as the
 * lexer has to distinguish between '++' and '+' early.  The following
 * sequence is thus tokenized as:
 *
 *	word		"a"
 *	postfix_op	"++"
 *	binary_op	"++"
 *	unary_op	"++"
 *	unary_op	"+"
 *	word		"b"
 *
 * See lexi.c, lexi, "case '+':".
 */
//indent input
void
joined_unary_and_binary_operators(void)
{
	if (a +++++++ b)
		return;
}
//indent end

//indent run
void
joined_unary_and_binary_operators(void)
{
	if (a++ ++ ++ +b)
		return;
}
//indent end


/*
 * Ensure that the result of the indentation does not depend on whether a
 * token from the input starts in column 1 or 9.
 *
 * See process_binary_op, ps.curr_col_1.
 */
//indent input
int col_1 //
= //
1;

int col_9 //
	= //
	9;
//indent end

//indent run
int		col_1		//
=				//
1;

int		col_9		//
=				//
9;
//indent end
