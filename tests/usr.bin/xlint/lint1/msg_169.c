/*	$NetBSD: msg_169.c,v 1.8 2023/07/07 06:03:31 rillig Exp $	*/
# 3 "msg_169.c"

// Test for message: precedence confusion possible: parenthesize! [169]

/* lint1-flags: -g -h -S -w -X 191,351 */

typedef _Bool bool;

void
confusing_shift_arith(unsigned a, unsigned b, unsigned c, unsigned char ch)
{
	unsigned con, okl, okr;

	/* expect+1: warning: precedence confusion possible: parenthesize! [169] */
	con = a + b << c;
	okl = (a + b) << c;
	okr = a + (b << c);

	/* expect+1: warning: precedence confusion possible: parenthesize! [169] */
	con = a << b + c;
	okl = (a << b) + c;
	okr = a << (b + c);

	/* expect+1: warning: precedence confusion possible: parenthesize! [169] */
	con = a - b >> c;
	okl = (a - b) >> c;
	okr = a - (b >> c);

	/* expect+1: warning: precedence confusion possible: parenthesize! [169] */
	con = a >> b - c;
	okl = (a >> b) - c;
	okr = a >> (b - c);

	// Parenthesizing the inner operands has no effect on the warning.
	/* expect+1: warning: precedence confusion possible: parenthesize! [169] */
	con = (a) + b << c;
	/* expect+1: warning: precedence confusion possible: parenthesize! [169] */
	con = a + (b) << c;
	/* expect+1: warning: precedence confusion possible: parenthesize! [169] */
	con = a + b << (c);

	// The usual arithmetic promotions have no effect on the warning.
	/* expect+1: warning: precedence confusion possible: parenthesize! [169] */
	con = ch + b << c;
	/* expect+1: warning: precedence confusion possible: parenthesize! [169] */
	con = a + ch << c;
	/* expect+1: warning: precedence confusion possible: parenthesize! [169] */
	con = a + b << ch;
}

void
confusing_logical(bool a, bool b, bool c)
{
	bool con, okl, okr, eql;

	eql = a && b && c;
	eql = a || b || c;

	/* expect+1: warning: precedence confusion possible: parenthesize! [169] */
	con = a && b || c;
	okl = (a && b) || c;
	okr = a && (b || c);

	/* expect+1: warning: precedence confusion possible: parenthesize! [169] */
	con = a || b && c;
	okl = (a || b) && c;
	okr = a || (b && c);
}

void
confusing_bitwise(unsigned a, unsigned b, unsigned c)
{
	bool con, okl, okr, eql;

	eql = a & b & c;
	eql = a | b | c;
	eql = a ^ b ^ c;

	/* expect+1: warning: precedence confusion possible: parenthesize! [169] */
	con = a | b ^ c;
	okl = (a | b) ^ c;
	okr = a | (b ^ c);

	/* expect+1: warning: precedence confusion possible: parenthesize! [169] */
	con = a | b & c;
	okl = (a | b) & c;
	okr = a | (b & c);

	/* expect+1: warning: precedence confusion possible: parenthesize! [169] */
	con = a ^ b | c;
	okl = (a ^ b) | c;
	okr = a ^ (b | c);

	/* expect+1: warning: precedence confusion possible: parenthesize! [169] */
	con = a ^ b & c;
	okl = (a ^ b) & c;
	okr = a ^ (b & c);

	/* expect+1: warning: precedence confusion possible: parenthesize! [169] */
	con = a & b | c;
	okl = (a & b) ^ c;
	okr = a & (b ^ c);

	/* expect+1: warning: precedence confusion possible: parenthesize! [169] */
	con = a & b ^ c;
	okl = (a & b) ^ c;
	okr = a & (b ^ c);

	/* expect+1: warning: precedence confusion possible: parenthesize! [169] */
	con = a & b + c;
	okl = (a & b) + c;
	okr = a & (b + c);

	/* expect+1: warning: precedence confusion possible: parenthesize! [169] */
	con = a - b | c;
	okl = (a - b) | c;
	okr = a - (b | c);

	// This looks like a binomial formula but isn't.
	/* expect+1: warning: precedence confusion possible: parenthesize! [169] */
	con = a ^ 2 - 2 * a * b + b ^ 2;

	// This isn't a binomial formula either since '^' means xor.
	con = (a ^ 2) - 2 * a * b + (b ^ 2);
}

void
constant_expressions(void)
{
	unsigned con;

	// The check for confusing precedence happens after constant folding.
	// Therefore the following lines do not generate warnings.
	con = 1 & 2 | 3;
	con = 4 << 5 + 6;
	con = 7 ^ 8 & 9;
}

void
cast_expressions(char a, char b, char c)
{
	unsigned con;

	// Adding casts to the leaf nodes doesn't change anything about the
	// confusing precedence.
	/* expect+1: warning: precedence confusion possible: parenthesize! [169] */
	con = (unsigned)a | (unsigned)b & (unsigned)c;
	/* expect+1: warning: precedence confusion possible: parenthesize! [169] */
	con = (unsigned)a & (unsigned)b | (unsigned)c;

	// Adding a cast around the whole calculation doesn't change the
	// precedence as well.
	/* expect+1: warning: precedence confusion possible: parenthesize! [169] */
	con = (unsigned)(a | b & c);

	// Adding a cast around an intermediate result groups the operands
	// of the main node, which prevents any confusion about precedence.
	con = (unsigned)a | (unsigned)(b & c);
	con = a | (unsigned)(b & c);
	con = (unsigned)(a | b) & (unsigned)c;
	con = (unsigned)(a | b) & c;
}

void
expected_precedence(int a, int b, int c)
{
	int ok;

	ok = a + b * c;
}

void
implicit_conversion_to_long(long la, int a)
{
	int ok;

	/* expect+1: warning: precedence confusion possible: parenthesize! [169] */
	ok = a & a | la;

	/*
	 * Before tree.c 1.132 from 2021-01-04, there was a typo in
	 * check_precedence_confusion that prevented the right-hand operand
	 * from being flagged as possibly confusing if there was an implicit
	 * conversion or an explicit cast between the main operator ('|') and
	 * the nested operator ('&').
	 */
	/* expect+1: warning: precedence confusion possible: parenthesize! [169] */
	ok = la | a & a;

	ok = (a & a) | la;	/* always ok */
	ok = la | (a & a);	/* always ok */

	/*
	 * Before tree.c 1.132, this expression didn't generate a warning
	 * because the right-hand operand was CVT, and there is no confusing
	 * precedence between BITOR and CVT.
	 *
	 * Since tree.c 1.132, this expression doesn't generate a warning
	 * because the right-hand operand is parenthesized.  There is no way
	 * to have the right operand casted and at the same time not
	 * parenthesized since the cast operator has higher precedence.
	 *
	 * In summary, there is no visible change, but the implementation is
	 * now works as intended.
	 */
	ok = la | (int)(a & a);	/* always ok */
}
