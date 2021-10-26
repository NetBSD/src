/* $NetBSD: token_binary_op.c,v 1.3 2021/10/26 22:00:38 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for binary operators like '+', '&&' and several others.
 *
 * Several binary operators can be used as unary operators as well, or in
 * other contexts.  An example for such an operator is '*', which can be a
 * multiplication, or pointer dereference, or pointer type declaration.
 */

/* See C99 6.4.6 */
#indent input
void
punctuators(void)
{
	int brackets = array[subscript];
	int parentheses = function(argument);
	int braces = { initializer };
	int period = structure.member;
	int arrow = structure->member;

	++prefix_increment;
	postfix_increment++;
	--prefix_decrement;
	postfix_decrement--;
	int *address = &lvalue;
	int bitwise_and = value & mask;
	int product = factor * factor;
	int dereferenced = *address;
	int positive = +number;
	int sum = number + number;
	int negative = -number;
	int difference = number - number;
	bool negated = !condition;

	int quotient = number / number;
	int modulo = number % number;
	int shifted_left = number << number;
	int shifted_right = number >> number;
	bool less_than = number < number;
	bool greater_than = number > number;
	bool less_equal = number <= number;
	bool greater_equal = number >= number;
	bool equal = number == number;
	bool unequal = number != number;
	int bitwise_exclusive_or = number ^ number;
	int bitwise_or = number | number;
	bool logical_and = condition && condition;
	bool logical_or = condition || condition;

	int conditional = condition ? number : number;

	/* combined assignment operators */
	number = (expression);
	number *= number;
	number /= number;
	number %= number;
	number += number;
	number -= number;
	number <<= number;
	number >>= number;
	number &= number;
	number ^= number;
	number |= number;

	number = function(argument1, argument2);
	number = function(argument), number;

	/* digraphs */
	number = array<:subscript:>;
	number = (int)<% initializer %>;
}
#indent end

#indent run -ldi0
void
punctuators(void)
{
	int brackets = array[subscript];
	int parentheses = function(argument);
/* $ XXX: The spaces around the initializer are gone. */
	int braces = {initializer};
	int period = structure.member;
	int arrow = structure->member;

	++prefix_increment;
	postfix_increment++;
	--prefix_decrement;
	postfix_decrement--;
	int *address = &lvalue;
	int bitwise_and = value & mask;
	int product = factor * factor;
	int dereferenced = *address;
	int positive = +number;
	int sum = number + number;
	int negative = -number;
	int difference = number - number;
	bool negated = !condition;

	int quotient = number / number;
	int modulo = number % number;
	int shifted_left = number << number;
	int shifted_right = number >> number;
	bool less_than = number < number;
	bool greater_than = number > number;
	bool less_equal = number <= number;
	bool greater_equal = number >= number;
	bool equal = number == number;
	bool unequal = number != number;
	int bitwise_exclusive_or = number ^ number;
	int bitwise_or = number | number;
	bool logical_and = condition && condition;
	bool logical_or = condition || condition;

	int conditional = condition ? number : number;

	/* combined assignment operators */
	number = (expression);
	number *= number;
	number /= number;
	number %= number;
	number += number;
	number -= number;
	number <<= number;
	number >>= number;
	number &= number;
	number ^= number;
	number |= number;

	number = function(argument1, argument2);
	number = function(argument), number;

	/* digraphs */
/* $ XXX: indent is confused by the digraphs for '[' and ']'. */
/* $ This probably doesn't matter since digraphs are not used in practice. */
number = array <:subscript:>;
	number = (int)<%initializer % >;
}
#indent end

#indent input
void
peculiarities(void)
{
	/*
	 * When indent tokenizes some of the operators, it allows for
	 * arbitrary repetitions of the operator character, followed by an
	 * arbitrary amount of '='.  This is used for operators like '&&' or
	 * '|||==='.
	 *
	 * Before 2021-03-07 22:11:01, the comment '//' was treated as an
	 * operator as well, and so was the comment '/////', leading to
	 * unexpected results; see comment-line-end.0 for more details.
	 *
	 * See lexi.c, lexi, "default:".
	 */
	if (a &&&&&&& b)
		return;
	if (a |||=== b)
		return;

	/*-
	 * For '+' and '-', this does not work since the lexer has to
	 * distinguish between '++' and '+' early.  The following sequence is
	 * thus tokenized as:
	 *
	 *	ident		'a'
	 *	postfix		'++'
	 *	binary_op	'++'
	 *	unary_op	'++'
	 *	unary_op	'+'
	 *	ident		'b'
	 *
	 * See lexi.c, lexi, "case '+':".
	 */
	if (a +++++++ b)
		return;
}
#indent end

#indent run -ldi0
void
peculiarities(void)
{
	/*
	 * When indent tokenizes some of the operators, it allows for
	 * arbitrary repetitions of the operator character, followed by an
	 * arbitrary amount of '='.  This is used for operators like '&&' or
	 * '|||==='.
	 *
	 * Before 2021-03-07 22:11:01, the comment '//' was treated as an
	 * operator as well, and so was the comment '/////', leading to
	 * unexpected results; see comment-line-end.0 for more details.
	 *
	 * See lexi.c, lexi, "default:".
	 */
	if (a &&&&&&& b)
		return;
	if (a |||=== b)
		return;

	/*-
	 * For '+' and '-', this does not work since the lexer has to
	 * distinguish between '++' and '+' early.  The following sequence is
	 * thus tokenized as:
	 *
	 *	ident		'a'
	 *	postfix		'++'
	 *	binary_op	'++'
	 *	unary_op	'++'
	 *	unary_op	'+'
	 *	ident		'b'
	 *
	 * See lexi.c, lexi, "case '+':".
	 */
	if (a++ ++ +++b)
		return;
}
#indent end


#indent input
char *(*fn)() = NULL;
char *(*fn)(int) = NULL;
char *(*fn)(int, int) = NULL;
#indent end

/* FIXME: The parameter '(int)' is wrongly interpreted as a type cast. */
/* FIXME: The parameter '(int, int)' is wrongly interpreted as a type cast. */
#indent run -di0
char *(*fn)() = NULL;
/* $ FIXME: Missing space before '='. */
char *(*fn)(int)= NULL;
/* $ FIXME: Missing space before '='. */
char *(*fn)(int, int)= NULL;
#indent end
