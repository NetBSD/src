/*	$NetBSD: d_c99_bool_strict.c,v 1.5 2021/01/15 22:04:27 rillig Exp $	*/
# 3 "d_c99_bool_strict.c"

/*
 * The option -T treats _Bool as incompatible with all other scalar types.
 * This means:
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
 * SB006: An expression is compatible with type _Bool if its main operator
 * returns type _Bool, or if the expression is an integer constant expression
 * with value 0 or 1.
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

/* lint1-extra-flags: -T */

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
	if (/*CONSTCOND*/2)	/* expect: 333 */
		return;

	/* Not allowed: There is no implicit conversion from scalar to bool. */
	if (i)			/* expect: 333 */
		return;
	if (i != 0)
		return;

	/* Not allowed: There is no implicit conversion from scalar to bool. */
	if (d)			/* expect: 333 */
		return;
	if (d != 0.0)
		return;

	/* Not allowed: There is no implicit conversion from scalar to bool. */
	if (p)			/* expect: 333 */
		return;
	if (p != (void *)0)
		return;

	/* Using a bool expression is allowed. */
	if (b)
		return;
}

void
SB002_operator_result_type(bool b)
{
	b = b;
	char c = b;		/* expect: 107 */
	int i = b;		/* expect: 107 */
	double d = b;		/* expect: 107 */
	void *p = b;		/* expect: 107 */

	/* The right-hand sides of these assignments are all ok. */
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
	 * The right-hand sides of these assignments are not ok, they
	 * implicitly convert from bool to int.
	 */
	i = !b;			/* expect: 107 */
	i = i == i;		/* expect: 107 */
	i = i != i;		/* expect: 107 */
	i = i < i;		/* expect: 107 */
	i = i <= i;		/* expect: 107 */
	i = i >= i;		/* expect: 107 */
	i = i > i;		/* expect: 107 */
	i = b && b;		/* expect: 107 */
	i = b || b;		/* expect: 107 */
}

int
SB003_operands(bool b, int i)
{

	/* The right-hand sides of these assignments are ok. */
	b = !b;
	b = b && b;
	b = b || b;

	/*
	 * The right-hand sides of these assignments implicitly convert from
	 * scalar to bool.
	 */
	b = !i;			/* expect: 330 */
	b = i && i;		/* expect: 331, 332 */
	b = i || i;		/* expect: 331, 332 */

	b = b && 0;
	b = 0 && b;
	b = b || 0;
	b = 0 || b;

	return i;
}

/*ARGSUSED*/
void
SB004_operators_and_bool_operands(bool b, unsigned u)
{
	b = !b;			/* ok */
	b = ~b;			/* expect: 335 */
	++b;			/* expect: 335 */
	--b;			/* expect: 335 */
	b++;			/* expect: 335 */
	b--;			/* expect: 335 */
	b = +b;			/* expect: 335 */
	b = -b;			/* expect: 335 */

	b = b * b;		/* expect: 336, 337 */
	b = b / b;		/* expect: 336, 337 */
	b = b % b;		/* expect: 336, 337 */
	b = b + b;		/* expect: 336, 337 */
	b = b - b;		/* expect: 336, 337 */
	b = b << b;		/* expect: 336, 337 */
	b = b >> b;		/* expect: 336, 337 */

	b = b < b;		/* expect: 336, 337 */
	b = b <= b;		/* expect: 336, 337 */
	b = b > b;		/* expect: 336, 337 */
	b = b >= b;		/* expect: 336, 337 */
	b = b == b;		/* ok */
	b = b != b;		/* ok */

	b = b & b;		/* ok */
	b = b ^ b;		/* ok */
	b = b | b;		/* ok */
	b = b && b;		/* ok */
	b = b || b;		/* ok */
	b = b ? b : b;		/* ok */

	b = b;			/* ok */
	b *= b;			/* expect: 336, 337 */
	b /= b;			/* expect: 336, 337 */
	b %= b;			/* expect: 336, 337 */
	b += b;			/* expect: 336, 337 */
	b -= b;			/* expect: 336, 337 */
	b <<= b;		/* expect: 336, 337 */
	b >>= b;		/* expect: 336, 337 */
	b &= b;			/* ok */
	b ^= b;			/* ok */
	b |= b;			/* ok */

	/* Operations with mixed types. */
	u = b * u;		/* expect: 336 */
	u = u * b;		/* expect: 337 */
	u = b / u;		/* expect: 336 */
	u = u / b;		/* expect: 337 */
	u = b % u;		/* expect: 336 */
	u = u % b;		/* expect: 337 */
	u = b + u;		/* expect: 336 */
	u = u + b;		/* expect: 337 */
	u = b - u;		/* expect: 336 */
	u = u - b;		/* expect: 337 */
	u = b << u;		/* expect: 336 */
	u = u << b;		/* expect: 337 */
	u = b >> u;		/* expect: 336 */
	u = u >> b;		/* expect: 337 */
	u = b ? u : u;		/* ok */
	u = b ? b : u;		/* expect: 107 */
	u = b ? u : b;		/* expect: 107 */
}

/*ARGSUSED*/
void
SB005_convert_from_bool_to_scalar(bool b)
{
	int i;
	unsigned u;
	double d;
	void *p;

	i = b;			/* expect: 107 */
	u = b;			/* expect: 107 */
	d = b;			/* expect: 107 */
	p = b;			/* expect: 107 */
}

enum SB006_bool_constant_expression {
	/* Ok: 0 is a boolean constant expression. */
	INT0 = 0 ? 100 : 101,

	/* Ok: 1 is a boolean constant expression. */
	INT1 = 1 ? 100 : 101,

	/* Not ok: 2 is not a boolean constant (neither 0 nor 1). */
	INT2 = 2 ? 100 : 101,	/* expect: 331 */

	/*
	 * The intermediate expression "2" has type int, which is not
	 * compatible with _Bool.  The expression "2 - 2" is an integer
	 * constant expression with value 0 and is thus a bool constant
	 * expression.  This particular case probably does not occur in
	 * practice.
	 */
	ARITH = (2 - 2) ? 100 : 101,

	/*
	 * These two variants of an expression can occur when a preprocessor
	 * macro is either defined to 1 or left empty, as in lint1/ops.def.
	 */
	BINARY_PLUS = (1 + 0) ? 100 : 101,
	UNARY_PLUS = (+0) ? 100 : 101,

	/* The main operator '>' has return type bool. */
	Q1 = (13 > 12) ? 100 : 101,

	/*
	 * The 7 is part of the integer constant expression, but it is
	 * irrelevant for the final result.  The expression in parentheses
	 * is an integer constant expression with value 1 and thus is a
	 * bool constant expression.
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

extern void
println(const char *);

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
 * 1073741824, which is more than what would fit into an unsigned char).
 * Even if an enum could be extended to larger types than int, this pattern
 * would work.
 */

/*
 * There is a crucial difference between a _Bool variable and an ordinary
 * integer variable.  C99 6.3.1.2 defines a conversion from an arbitrary
 * scalar value to _Bool as equivalent to (value != 0 ? 1 : 0).  This means
 * that even if _Bool is implemented as an 8-bit unsigned integer, assigning
 * 256 to it would still result in the value 1 being stored.  Storing 256 in
 * an ordinary 8-bit unsigned integer would result in the value 0 being
 * stored.  See the test d_c99_bool.c for more details.
 *
 * Because of this, expressions like (flags & FLAG28) are only allowed in
 * bool context if they are guaranteed not to be truncated, even if the
 * result were to be stored in a plain unsigned integer.
 */

/*ARGSUSED*/
void
SB007_allow_flag_test_on_bit_set_enums(enum Flags flags)
{
	bool b;

	/*
	 * FLAG0 has the value 1 and thus can be stored in a bool variable
	 * without truncation.  Nevertheless this special case is not allowed
	 * because it would be too confusing if FLAG0 would work and all the
	 * other flags wouldn't.
	 */
	b = flags & FLAG0;	/* expect: 107 */

	/*
	 * Assuming that FLAG1 is set in flags, a _Bool variable stores this
	 * as 1, as defined by C99 6.3.1.2.  A uint8_t variable would store
	 * it as 2, as that is the integer value of FLAG1.  Since FLAG1 fits
	 * in a uint8_t, no truncation takes place.
	 */
	b = flags & FLAG1;	/* expect: 107 */

	/*
	 * In a _Bool variable, FLAG28 is stored as 1, since it is unequal to
	 * zero.  In a uint8_t, the stored value would be 0 since bit 28 is
	 * out of range for a uint8_t and thus gets truncated.
	 */
	b = flags & FLAG28;	/* expect: 107 */
}

/* A bool bit field is compatible with bool. Other bit fields are not. */

struct flags {
	bool bool_flag: 1;
	unsigned uint_flag: 1;
};

/*ARGSUSED*/
void
SB008_bit_fields(const struct flags *flags)
{
	bool b;

	b = flags->bool_flag;	/* ok */
	b = flags->uint_flag;	/* expect: 107 */
	flags->bool_flag = b;
	flags->uint_flag = b;	/* expect: 107 */
}

/* Test implicit conversion when returning a value from a function. */

/*ARGSUSED*/
bool
returning_bool(bool b, int i, const char *p)
{
	if (i > 0)
		return b;	/* ok */
	if (i < 0)
		return i;	/* expect: 211 */
	return p;		/* expect: 211 */
}

/*ARGSUSED*/
char
returning_char(bool b, int i, const char *p)
{
	if (i > 0)
		return b;	/* expect: 211 */
	if (i < 0)
		return i;	/* XXX: narrowing conversion */
	return p;		/* expect: 183 */
}

bool
return_constant_false(void)
{
	return 0;
}

bool
return_constant_true(void)
{
	return 1;
}

bool
return_invalid_integer(void)
{
	return 2;		/* expect: 211 */
}

/* Test passing arguments to a function. */

extern void
taking_arguments(bool, int, const char *, ...);

void
passing_arguments(bool b, int i, const char *p)
{
	/* No conversion necessary. */
	taking_arguments(b, i, p);

	/* Implicitly converting bool to other scalar types. */
	taking_arguments(b, b, b);	/* expect: 334, 334 */

	/* Implicitly converting int to bool (arg #1). */
	taking_arguments(i, i, i);	/* expect: 334, 154 */

	/* Implicitly converting pointer to bool (arg #1). */
	taking_arguments(p, p, p);	/* expect: 334, 154 */

	/* Passing bool as vararg. */
	taking_arguments(b, i, p, b, i, p); /* expect: arg#4 */ // TODO

	/* Passing a bool constant. */
	taking_arguments(0, i, p);

	/* Passing a bool constant. */
	taking_arguments(1, i, p);

	/* Trying to pass an invalid integer. */
	taking_arguments(2, i, p);	/* expect: 334 */
}

/*
 * This is just normal access to a bool member of a struct, to ensure that
 * these produce no errors.
 */
void
struct_access_operators(void)
{
	struct bool_struct {
		bool b;
	};

	/* Initialize and assign using boolean constants. */
	bool b = 0;
	b = 1;

	/* Access a struct member using the '.' operator. */
	struct bool_struct bs = { 1 };
	b = bs.b;
	bs.b = b;
	bs.b = 0;

	/* Access a struct member using the '->' operator. */
	struct bool_struct *bsp = &bs;
	b = bsp->b;
	bsp->b = b;
	bsp->b = 0;

	/* Taking the address of a bool lvalue. */
	bool *bp;
	bp = &b;
	*bp = b;
	b = *bp;
}

/*
 * Comparing a _Bool expression to 0 or 1 is redundant.  It may come from
 * an earlier version of the code, before it got migrated to using _Bool.
 * Usually, bool expressions are used directly as control expressions or
 * as argument to the boolean operators such as '!', '&&', '||'.
 *
 * Since lint steps in after the C preprocessor, it has no chance of seeing
 * the original source code, which may well have been "b == false" instead
 * of "b == 0".
 */
bool
compare_var_with_constant(bool b)
{
	bool t1 = b == 0;
	bool t2 = t1 != 0;
	bool t3 = t2 == 1;
	bool t4 = t3 != 1;
	return t4 ^ t3;
}

bool
SB003_operand_comma(bool b)
{
	b = (b, !b);		/* FIXME *//* expect: 336, 337 */
	return b;
}
