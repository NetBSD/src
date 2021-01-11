/*	$NetBSD: d_c99_bool_strict.c,v 1.3 2021/01/11 00:28:28 rillig Exp $	*/
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
 *
 * SB007: Expressions like "flags & FLAG" are compatible with _Bool if
 * they appear in a context where they are immediately compared to zero.
 * Assigning to a _Bool variable does not count as such a context, to
 * allow programs to be compiled without silent changes on a compiler that
 * is lacking the special _Bool type.
 *
 * SB008: Bit fields in struct may be based on _Bool.  These bit fields
 * typically have type _Bool:1 and can be converted to _Bool and back.
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

/*
 * An efficient implementation technique for a collection of boolean flags
 * is an enum.  The enum declaration groups the available constants, and as
 * of 2020, compilers such as GCC and Clang have basic support for detecting
 * type mismatches on enums.
 */

enum Flags {
	FLAG0 = 1 << 0,
	FLAG1 = 1 << 1,
	FLAG28 = 1 << 28
};

/*
 * The usual way to query one of the flags is demonstrated below.
 */

extern void println(const char *);

void
query_flag_from_enum_bit_set(enum Flags flags)
{

	if (flags & FLAG0)
		println("FLAG0 is set");

	if ((flags & FLAG1) != 0)
		println("FLAG1 is set");

	if ((flags & (FLAG0 | FLAG1)) == (FLAG0 | FLAG1))
		println("FLAG0 and FLAG1 are both set");
	if (flags & FLAG0 && flags & FLAG1)
		println("FLAG0 and FLAG1 are both set");

	if ((flags & (FLAG0 | FLAG1)) != 0)
		println("At least one of FLAG0 and FLAG1 is set");

	if (flags & FLAG28)
		println("FLAG28 is set");
}

/*
 * In all the above conditions (or controlling expressions, as the C standard
 * calls them), the result of the operator '&' is compared against 0.  This
 * makes this pattern work, no matter whether the bits are in the low-value
 * range or in the high-value range (such as FLAG28, which has the value
 * 1073741824, which is more than what fits into an unsigned char).  Even
 * if an enum could be extended to larger types than int, this pattern
 * would work.
 */

/*
 * There is a crucial difference between a _Bool variable and an ordinary
 * integer variable though.  C99 6.3.1.2 defines a conversion from an
 * arbitrary scalar type to _Bool as (value != 0 ? 1 : 0).  This means that
 * even if _Bool is implemented as an 8-bit unsigned integer, assigning 256
 * to it would still result in the value 1 being stored.  Storing 256 in an
 * ordinary 8-bit unsigned integer would result in the value 0 being stored.
 * See the test d_c99_bool.c for more details.
 *
 * Because of this, expressions like (flags & FLAG28) are only allowed in
 * bool context if they are guaranteed not to be truncated, even if the
 * result were to be stored in a plain unsigned integer.
 */

void
SB007_allow_flag_test_on_bit_set_enums(enum Flags flags)
{
	bool b;

	/*
	 * FLAG0 has the value 1 and can therefore be stored in a bool
	 * variable without truncation.  Nevertheless this special case
	 * is not allowed because it would be too confusing if FLAG0 would
	 * work and all the other flags wouldn't.
	 */
	b = flags & FLAG0;

	/*
	 * Assuming that FLAG1 is set in flags, a _Bool variable stores this
	 * as 1, as defined by C99 6.3.1.2.  An unsigned char variable would
	 * store it as 2, as that is the integer value of FLAG1.  Since FLAG1
	 * fits in an unsigned char, no truncation takes place.
	 */
	b = flags & FLAG1;

	/*
	 * In a _Bool variable, FLAG28 is stored as 1, as above.  In an
	 * unsigned char, the stored value would be 0 since bit 28 is out of
	 * range for an unsigned char, which usually has 8 significant bits.
	 */
	b = flags & FLAG28;
}

/* A bool bit field is compatible with bool. Other bit fields are not. */

struct flags {
	bool bool_flag: 1;
	unsigned uint_flag: 1;
};

void
SB008_flags_from_bit_fields(const struct flags *flags)
{
	bool b;

	b = flags->bool_flag;	/* ok */
	b = flags->uint_flag;	/* not ok */
}

/* Test implicit conversion when returning a value from a function. */

bool
returning_bool(bool b, int i, const char *p)
{
	if (i > 0)
		return b;	/* ok */
	if (i < 0)
		return i;	/* not ok */
	return p;		/* not ok */
}

char
returning_char(bool b, int i, const char *p)
{
	if (i > 0)
		return b;	/* not ok */
	if (i < 0)
		return i;	/* not ok, but not related to bool */
	return p;		/* not ok */
}

/* Test passing arguments to a function. */

extern void taking_arguments(bool, int, const char *, ...);

void
passing_arguments(bool b, int i, const char *p)
{
	/* No conversion necessary. */
	taking_arguments(b, i, p);

	/* Implicitly converting bool to other scalar types. */
	taking_arguments(b, b, b);

	/* Implicitly converting int to bool (arg #1). */
	taking_arguments(i, i, i);

	/* Implicitly converting pointer to bool (arg #1). */
	taking_arguments(p, p, p);

	/* Passing bool as vararg. */
	taking_arguments(b, i, p, b, i, p);
}
