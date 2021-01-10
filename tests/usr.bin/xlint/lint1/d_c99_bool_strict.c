/*	$NetBSD: d_c99_bool_strict.c,v 1.2 2021/01/10 21:45:50 rillig Exp $	*/
# 3 "d_c99_bool_strict.c"

/*
 * Experimental feature:  allow to treat _Bool as incompatible with all
 * scalar types.  This means:
 *
 * SB001: Controlling expressions in 'if', 'while', 'for', '?:' must be of
 * type _Bool instead of scalar.
 *
 * SB002: The operators '!', '==', '!=', '<', '<=', '>=', '>', '&&', '||'
 * return _Bool instead of int.
 *
 * SB003: The operators '!', '&&', '||' take _Bool instead of scalar.
 *
 * SB004: The only operators that take _Bool are '!', '==', '!=',
 * '&', '^', '|', '&&', '||', '?', ':', '=', '&=', '^=', '|='.
 *
 * SB005: There is no implicit conversion from _Bool to any other type.
 *
 * SB006: A constant integer expression is compatible with type _Bool if
 * it is an integer constant with value 0 or 1, or if the result type of
 * its main operator is _Bool.
 */

// Not yet implemented: /* lint1-extra-flags: -T */

/*
 * The header <stdbool.h> defines the macros bool = _Bool, false = 0 and
 * true = 1.  Therefore, constant expressions of integer type have to be
 * regarded as possible boolean constants if their value is either 0 or 1.
 * At this point of the translation, the preprocessor has already removed
 * the words "false" and "true" from the source code.
 */

/*
 * Using a typedef for bool does not hurt the checks, they all use the
 * underlying basic type (see tspec_t), which is BOOL.
 */
typedef _Bool bool;

void
SB001_controlling_expression(bool b, int i, double d, const void *p)
{

	/* Fine due to SB006. */
	if (/*CONSTCOND*/0)
		return;

	/* Fine due to SB006. */
	if (/*CONSTCOND*/1)
		return;

	/* Not allowed: 2 is not a boolean expression. */
	if (/*CONSTCOND*/2)
		return;

	/* Not allowed: There is no implicit conversion from scalar to bool. */
	if (i)
		return;
	if (i != 0)
		return;

	/* Not allowed: There is no implicit conversion from scalar to bool. */
	if (d)
		return;
	if (d != 0.0)
		return;

	/* Not allowed: There is no implicit conversion from scalar to bool. */
	if (p)
		return;
	if (p != (void *)0)
		return;

	/* Using a bool expression is allowed. */
	if (b)
		return;
}

void
SB002_operator_result(bool b)
{
	b = b;
	char c = b;
	int i = b;
	double d = b;
	void *p = b;

	/* These assignments are all ok. */
	b = !b;
	b = i == i;
	b = i != i;
	b = i < i;
	b = i <= i;
	b = i >= i;
	b = i > i;
	b = b && b;
	b = b || b;

	/*
	 * These assignments are not ok, they implicitly convert from bool
	 * to int.
	 */
	i = !b;
	i = i == i;
	i = i != i;
	i = i < i;
	i = i <= i;
	i = i >= i;
	i = i > i;
	i = b && b;
	i = b || b;
}

void
SB003_operands(bool b, int i)
{

	/* These assignments are ok. */
	b = !b;
	b = b && b;
	b = b || b;

	/* These assignments implicitly convert from scalar to bool. */
	b = !i;
	b = i && i;
	b = i || i;
}

void
SB004_non_bool_operands(bool b, unsigned u)
{
	b = !b;			/* ok */
	b = ~b;			/* not ok */
	++b;			/* not ok */
	--b;			/* not ok */
	b++;			/* not ok */
	b--;			/* not ok */
	b = +b;			/* not ok */
	b = -b;			/* not ok */

	b = b * b;		/* not ok */
	b = b / b;		/* not ok */
	b = b % b;		/* not ok */
	b = b + b;		/* not ok */
	b = b - b;		/* not ok */
	b = b << b;		/* not ok */
	b = b >> b;		/* not ok */

	b = b < b;		/* not ok */
	b = b <= b;		/* not ok */
	b = b > b;		/* not ok */
	b = b >= b;		/* not ok */
	b = b == b;		/* ok */
	b = b != b;		/* ok */

	b = b & b;		/* ok */
	b = b ^ b;		/* ok */
	b = b | b;		/* ok */
	b = b && b;		/* ok */
	b = b || b;		/* ok */
	b = b ? b : b;		/* ok */

	b = b;			/* ok */
	b *= b;			/* not ok */
	b /= b;			/* not ok */
	b %= b;			/* not ok */
	b += b;			/* not ok */
	b -= b;			/* not ok */
	b <<= b;		/* not ok */
	b >>= b;		/* not ok */
	b &= b;			/* ok */
	b ^= b;			/* ok */
	b |= b;			/* ok */

	/* Operations with mixed types. */
	u = b * u;		/* not ok */
	u = u * b;		/* not ok */
	u = b / u;		/* not ok */
	u = u / b;		/* not ok */
	u = b % u;		/* not ok */
	u = u % b;		/* not ok */
	u = b + u;		/* not ok */
	u = u + b;		/* not ok */
	u = b - u;		/* not ok */
	u = u - b;		/* not ok */
	u = b << u;		/* not ok */
	u = u << b;		/* not ok */
	u = b >> u;		/* not ok */
	u = u >> b;		/* not ok */
	u = b ? u : u;		/* ok */
	u = b ? b : u;		/* not ok */
	u = b ? u : b;		/* not ok */
}

void
SB005_convert_from_bool_to_scalar(bool b)
{
	int i;
	unsigned u;
	double d;
	void *p;

	i = b;			/* not ok */
	u = b;			/* not ok */
	d = b;			/* not ok */
	p = b;			/* not ok */
}

enum SB006_bool_constant_expression {
	/* Ok: 0 is a boolean constant expression. */
	INT0 = 0 ? 100 : 101,

	/* Ok: 1 is a boolean constant expression. */
	INT1 = 1 ? 100 : 101,

	/* Not ok: 2 is not a boolean constant (neither 0 nor 1). */
	INT2 = 2 ? 100 : 101,

	/*
	 * Not ok: the intermediate expression "2 - 2" has return type
	 * scalar, not bool.  It is irrelevant that the final result
	 * is 0, which would be a boolean constant.
	 */
	ARITH = (2 - 2) ? 100 : 101,

	/*
	 * Ok: The 13 and 12 are not boolean expressions, but they
	 * are not in the calculation path that leads to the final
	 * result.  The important point is that the operator '>' has
	 * return type bool.
	 */
	Q1 = (13 > 12) ? 100 : 101,

	/*
	 * Not ok: The 7 is irrelevant for the final result of the
	 * expression, yet it turns the result type of the operator
	 * '?:' to be int, not bool.
	 */
	Q2 = (13 > 12 ? 1 : 7) ? 100 : 101,

	BINAND = 0 & 1,		/* ok */

	BINXOR = 0 ^ 1,		/* ok */

	BINOR = 0 | 1,		/* ok */

	LOGOR = 0 || 1,		/* ok */

	LOGAND = 0 && 1,	/* ok */
};

enum BitSet {
	ONE = 1 << 0,
	TWO = 1 << 1,
	FOUR = 1 << 2
};

/*
 * It is debatable whether it is a good idea to allow expressions like these
 * for _Bool.  The strict rules above ensure that the code works in the same
 * way whether or not the special rule C99 6.3.1.2 is active or not.
 *
 * If the code were to switch away from the C99 bool type to an ordinary
 * unsigned integer type, the behavior might silently change.  Because the
 * rule C99 6.3.1.2 is no longer active in that case, high bits of the enum
 * constant may get lost, thus evaluating to false even though a bit is set.
 *
 * It's probably better to not allow this kind of expressions, even though
 * it may be popular, especially in usr.bin/make.
 */
int
S007_allow_flag_test_on_bit_set_enums(enum BitSet bs)
{
	if (bs & ONE)
		return 1;
	if (!(bs & TWO))
		return 2;
	if (bs & FOUR)
		return 2;
	return 4;
}
