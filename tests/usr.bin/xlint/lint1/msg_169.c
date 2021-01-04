/*	$NetBSD: msg_169.c,v 1.3 2021/01/04 22:41:56 rillig Exp $	*/
# 3 "msg_169.c"

// Test for message: precedence confusion possible: parenthesize! [169]

/* lint1-flags: -g -h -S -w */

typedef _Bool bool;

void
confusing_shift_arith(unsigned a, unsigned b, unsigned c, unsigned char ch)
{
	unsigned con, okl, okr;

	con = a + b << c;
	okl = (a + b) << c;
	okr = a + (b << c);

	con = a << b + c;
	okl = (a << b) + c;
	okr = a << (b + c);

	con = a - b >> c;
	okl = (a - b) >> c;
	okr = a - (b >> c);

	con = a >> b - c;
	okl = (a >> b) - c;
	okr = a >> (b - c);

	// Parenthesizing the inner operands has no effect on the warning.
	con = (a) + b << c;
	con = a + (b) << c;
	con = a + b << (c);

	// The usual arithmetic promotions have no effect on the warning.
	con = ch + b << c;
	con = a + ch << c;
	con = a + b << ch;
}

void
confusing_logical(bool a, bool b, bool c)
{
	bool con, okl, okr, eql;

	eql = a && b && c;
	eql = a || b || c;

	con = a && b || c;
	okl = (a && b) || c;
	okr = a && (b || c);

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

	con = a | b ^ c;
	okl = (a | b) ^ c;
	okr = a | (b ^ c);

	con = a | b & c;
	okl = (a | b) & c;
	okr = a | (b & c);

	con = a ^ b | c;
	okl = (a ^ b) | c;
	okr = a ^ (b | c);

	con = a ^ b & c;
	okl = (a ^ b) & c;
	okr = a ^ (b & c);

	con = a & b | c;
	okl = (a & b) ^ c;
	okr = a & (b ^ c);

	con = a & b ^ c;
	okl = (a & b) ^ c;
	okr = a & (b ^ c);

	con = a & b + c;
	okl = (a & b) + c;
	okr = a & (b + c);

	con = a - b | c;
	okl = (a - b) | c;
	okr = a - (b | c);

	// This looks like a binomial formula but isn't.
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
	con = (unsigned)a | (unsigned)b & (unsigned)c;
	con = (unsigned)a & (unsigned)b | (unsigned)c;

	// Adding a cast around the whole calculation doesn't change the
	// precedence as well.
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

// TODO: add a test with unsigned long instead of unsigned, trying to
//  demonstrate that the typo in check_precedence_confusion actually has an
//  effect.
