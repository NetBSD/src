/* $NetBSD: token_binary_op.c,v 1.11 2022/04/23 09:35:26 rillig Exp $ */

/*
 * Tests for binary operators like '+', '&&' and several others.
 *
 * Several binary operators can be used as unary operators as well, or in
 * other contexts.  An example for such an operator is '*', which can be a
 * multiplication, or pointer dereference, or pointer type declaration.
 */

/* TODO: split into separate tests */

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

	number = function(argument1, argument2);
	number = function(argument), number;

	/* digraphs */
/* $ XXX: indent is confused by the digraphs for '[' and ']'. */
/* $ This probably doesn't matter since digraphs are not used in practice. */
number = array <:subscript:>;
	number = (int)<%initializer % >;
}
#indent end
/* TODO: move digraphs into separate test */
/* TODO: test trigraphs, which are as unusual as digraphs */
/* TODO: test digraphs and trigraphs in string literals, just for fun */


/*
 * When indent tokenizes some of the operators, it allows for
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
#indent input
void
long_run_of_operators(void)
{
	if (a &&&&&&& b)
		return;
	if (a |||=== b)
		return;
}
#indent end

#indent run-equals-input


/*
 * For '+' and '-', this does not work since the lexer has to
 * distinguish between '++' and '+' early.  The following sequence is
 * thus tokenized as:
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
#indent input
void
joined_unary_and_binary_operators(void)
{
	if (a +++++++ b)
		return;
}
#indent end

#indent run
void
joined_unary_and_binary_operators(void)
{
	if (a++ ++ ++ +b)
		return;
}
#indent end


/*
 * Before NetBSD indent.c 1.178 from 2021-10-29, indent removed the blank
 * before the '=', in the second and third of these function pointer
 * declarations. This was because indent interpreted the prototype parameters
 * 'int' and 'int, int' as type casts, which doesn't make sense at all. Fixing
 * this properly requires large style changes since indent is based on simple
 * heuristics all over. This didn't change in indent.c 1.178; instead, the
 * rule for inserting a blank before a binary operator was changed to always
 * insert a blank, except at the beginning of a line.
 */
#indent input
char *(*fn)() = NULL;
char *(*fn)(int) = NULL;
char *(*fn)(int, int) = NULL;
#indent end

/* XXX: The parameter '(int)' is wrongly interpreted as a type cast. */
/* XXX: The parameter '(int, int)' is wrongly interpreted as a type cast. */
#indent run-equals-input -di0


/*
 * Ensure that the result of the indentation does not depend on whether a
 * token from the input starts in column 1 or 9.
 *
 * See process_binary_op, ps.curr_col_1.
 */
#indent input
int col_1 //
= //
1;

int col_9 //
	= //
	9;
#indent end

#indent run
int		col_1		//
=				//
1;

int		col_9		//
=				//
9;
#indent end
