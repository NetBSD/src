/*	$NetBSD: d_c99_bool_strict.c,v 1.30 2021/07/04 07:09:39 rillig Exp $	*/
# 3 "d_c99_bool_strict.c"

/*
 * The option -T treats _Bool as incompatible with all other scalar types.
 * This is implemented by the following rules:
 *
 * strict-bool-typedef:
 *	The type _Bool is compatible with any typedef of _Bool.
 *
 *	Note: Since <stdbool.h> defines bool as textual alias of _Bool,
 *	having another typedef for bool is unusual.
 *
 * strict-bool-constant:
 *	There are 2 bool constants named false and true.
 *	No other constants are compatible with type _Bool.
 *
 *	Note: Internally these constants are named __lint_false and
 *	__lint_true.
 *
 * strict-bool-bit-field:
 *	A struct or union member that is a bit field with underlying type
 *	bool is compatible with plain bool.
 *
 * strict-bool-conversion:
 *	There is no implicit conversion between _Bool and any other type.
 *
 * strict-bool-controlling-expression:
 *	Controlling expressions in 'if', 'while', 'for', '?:' must be of
 *	type bool.
 *
 * strict-bool-operand-unary:
 *	Operator	bool?	scalar?
 *	!		yes	-
 *	&		yes	yes
 *	The other unary operators do not accept bool operands.
 *
 * strict-bool-operand-binary:
 *	Operator	left:	bool?	other?	right:	bool?	other?
 *	.			-	yes		yes	yes
 *	->			-	yes		yes	yes
 *	<=, <, >=, >		-	yes		-	yes
 *	==, !=			yes	yes		yes	yes
 *	&			yes	yes		yes	yes
 *	^			yes	yes		yes	yes
 *	|			yes	yes		yes	yes
 *	&&			yes	-		yes	-
 *	||			yes	-		yes	-
 *	?			yes	-		yes	yes
 *	:			yes	yes		yes	yes
 *	=			yes	yes		yes	yes
 *	&=, ^=, |=		yes	yes		yes	yes
 *	,			yes	yes		yes	yes
 *	The other binary operators do not accept bool operands.
 *
 * strict-bool-operator-result:
 *	The result type of the operators '!', '<', '<=', '>', '>=',
 *	'==', '!=', '&&', '||' is _Bool instead of int.
 *
 * strict-bool-bitwise-and:
 *	Expressions of the form "flags & FLAG" are compatible with _Bool if
 *	the left operand has enum type, the right operand is an integer
 *	constant and the resulting value is used in a context where it is
 *	implicitly and immediately compared to zero.
 *
 *	Note: An efficient implementation technique for a collection of bool
 *	flags is an enum.  The enum declaration groups the available
 *	constants, and as of 2020, compilers such as GCC and Clang have basic
 *	support for detecting type mismatches on enums.
 *
 *	Note: Examples for such contexts are controlling expressions or the
 *	operands of the operators '!', '&&', '||'.
 *
 *	Note: Counterexamples for contexts are assignments to a bool variable.
 *
 *	Note: These rules ensure that conforming code can be compiled without
 *	change in behavior using old compilers that implement bool as an
 *	ordinary integer type, without the special rule C99 6.3.1.2.
 *
 *	Note: There is a crucial difference between a _Bool variable and an
 *	ordinary integer variable.  C99 6.3.1.2 defines a conversion from an
 *	arbitrary scalar value to _Bool as equivalent to (value != 0 ? 1 : 0).
 *	This means that even if _Bool is implemented as an 8-bit unsigned
 *	integer, assigning 256 to it would still result in the value 1 being
 *	stored.  Storing 256 in an ordinary 8-bit unsigned integer would
 *	result in the value 0 being stored.  See the test d_c99_bool.c for
 *	more details.
 */

/*
 * The header <stdbool.h> defines the macros bool = _Bool, false = 0 and
 * true = 1.  Without further hacks, this would mean that constant expressions
 * of integer type have to be regarded as possible boolean constants if their
 * value is either 0 or 1.
 *
 * This would not help in migrating old code to use bool consistently.
 * Therefore lint provides its own <stdbool.h> header that expands false to
 * __lint_false and true to __lint_true, two predefined constant expressions.
 */

/* lint1-extra-flags: -hT */

/*
 * strict-bool-typedef
 */

/*
 * Using a typedef for bool does not hurt the checks, they all use the
 * underlying basic type (see tspec_t), which is BOOL.
 */
typedef _Bool bool;

extern void accept_bool(bool);
extern void println(const char *);
extern void take_arguments(bool, int, const char *, ...);
extern void do_nothing(void);

/*
 * strict-bool-constant
 */

void
strict_bool_constant(void)
{
	accept_bool(__lint_false);
	accept_bool(__lint_true);
	accept_bool(0);		/* expect: 334 */
	accept_bool(1);		/* expect: 334 */
	accept_bool(2);		/* expect: 334 */
}

enum strict_bool_constant_expressions {
	/* Ok: __lint_false is a boolean constant expression. */
	FALSE = __lint_false ? 100 : 101,	/* expect: 161 */

	/* Ok: __lint_true is a boolean constant expression. */
	TRUE = __lint_true ? 100 : 101,		/* expect: 161 */

	/* Not ok: an integer is not a boolean constant expression. */
	INT0 = 0 ? 100 : 101,	/* expect: 331 */

	/* Not ok: an integer is not a boolean constant expression. */
	INT1 = 1 ? 100 : 101,	/* expect: 331 */

	/* Not ok: 2 is not a boolean constant. */
	INT2 = 2 ? 100 : 101,	/* expect: 331 */

	/* Not ok: compound integer expressions are not bool. */
	ARITH = (2 - 2) ? 100 : 101,	/* expect: 331 */

	/*
	 * Without strict bool mode, these two variants of an expression can
	 * occur when a preprocessor macro is either defined to 1 or left
	 * empty (since C99), as in lint1/ops.def.
	 *
	 * In strict bool mode, the resulting expression can be compared
	 * against 0 to achieve the same effect (so +0 != 0 or 1 + 0 != 0).
	 */
	BINARY_PLUS = (1 + 0) ? 100 : 101, /* expect: 331 */
	UNARY_PLUS = (+0) ? 100 : 101,	/* expect: 331 */

	/* The main operator '>' has return type bool. */
	Q1 = (13 > 12) ? 100 : 101,		/* expect: 161 */

	/*
	 * The parenthesized expression has type int and thus cannot be
	 * used as the controlling expression in the '?:' operator.
	 */
	Q2 = (13 > 12 ? 1 : 7) ? 100 : 101,	/* expect: 161 *//* expect: 331 */

	BINAND_BOOL = __lint_false & __lint_true, /* expect: 55 */
	BINAND_INT = 0 & 1,

	BINXOR_BOOL = __lint_false ^ __lint_true, /* expect: 55 */
	BINXOR_INT = 0 ^ 1,

	BINOR_BOOL = __lint_false | __lint_true, /* expect: 55 */
	BINOR_INT = 0 | 1,

	LOGOR_BOOL = __lint_false || __lint_true, /* expect: 161 *//* expect: 55 */
	LOGOR_INT = 0 || 1,	/* expect: 331 *//* expect: 332 */

	LOGAND_BOOL = __lint_false && __lint_true, /* expect: 161 *//* expect: 55 */
	LOGAND_INT = 0 && 1,	/* expect: 331 *//* expect: 332 */
};

/*
 * strict-bool-bit-fields
 */

void
strict_bool_bit_fields(void)
{
	struct flags {
		bool bool_flag: 1;
		unsigned uint_flag: 1;
	};

	struct flags flags = { __lint_false, 0 };
	struct flags *flags_ptr = &flags;
	bool b;

	b = flags.bool_flag;
	b = flags.uint_flag;		/* expect: 107 */
	flags.bool_flag = b;
	flags.uint_flag = b;		/* expect: 107 */

	b = flags_ptr->bool_flag;
	b = flags_ptr->uint_flag;	/* expect: 107 */
	flags_ptr->bool_flag = b;
	flags_ptr->uint_flag = b;	/* expect: 107 */
}

void
strict_bool_bit_fields_operand_conversion(void)
{
	struct s {
		bool ordinary;
		bool bit_field: 1;
	};

	struct s s = { 0 > 0 };

	s.ordinary = s.ordinary | s.ordinary;
	s.bit_field = s.bit_field | s.bit_field;
}

/*
 * strict-bool-conversion
 */

bool
strict_bool_conversion_return_false(void)
{
	return __lint_false;
}

bool
strict_bool_conversion_return_true(void)
{
	return __lint_true;
}

bool
strict_bool_conversion_return_bool(bool b)
{
	return b;
}

bool
strict_bool_conversion_return_0(void)
{
	return 0;		/* expect: 211 */
}

bool
strict_bool_conversion_return_1(void)
{
	return 1;		/* expect: 211 */
}

bool
strict_bool_conversion_return_2(void)
{
	return 2;		/* expect: 211 */
}

bool
strict_bool_conversion_return_pointer(const void *p) /* expect: 231 */
{
	return p;		/* expect: 211 */
}

char
strict_bool_conversion_return_false_as_char(void)
{
	return __lint_false;	/* expect: 211 */
}

char
strict_bool_conversion_return_true_as_char(void)
{
	return __lint_true;	/* expect: 211 */
}


void
strict_bool_conversion_function_argument(void)
{
	accept_bool(__lint_false);
	accept_bool(__lint_true);
}

void
strict_bool_conversion_function_argument_pass(bool b, int i, const char *p)
{
	/* No conversion necessary. */
	take_arguments(b, i, p);

	/* Implicitly converting bool to other scalar types. */
	take_arguments(b, b, b);	/* expect: 334 *//* expect: 334 */

	/* Implicitly converting int to bool (arg #1). */
	take_arguments(i, i, i);	/* expect: 334 *//* expect: 154 */

	/* Implicitly converting pointer to bool (arg #1). */
	take_arguments(p, p, p);	/* expect: 334 *//* expect: 154 */

	/* Passing bool as vararg. */
	take_arguments(b, i, p, b, i, p); /* TODO: expect: arg#4 */

	/* Passing a bool constant. */
	take_arguments(__lint_false, i, p);

	/* Passing a bool constant. */
	take_arguments(__lint_true, i, p);

	/* Trying to pass integer constants. */
	take_arguments(0, i, p);	/* expect: 334 */
	take_arguments(1, i, p);	/* expect: 334 */
	take_arguments(2, i, p);	/* expect: 334 */
}

void
strict_bool_conversion_between_bool_and_int(void)
{
	bool b;
	int i;

	b = 0;			/* expect: 107 */
	b = __lint_false;
	b = 1;			/* expect: 107 */
	b = __lint_true;

	i = 0;
	i = __lint_false;	/* expect: 107 */
	i = 1;
	i = __lint_true;	/* expect: 107 */

	i = b;			/* expect: 107 */
	b = i;			/* expect: 107 */
}

void
strict_bool_conversion_from_bool_to_scalar(bool b) /* expect: 231 */
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

/*
 * strict-bool-controlling-expression:
 *	Controlling expressions in 'if', 'while', 'for', '?:' must be of
 *	type bool.
 */

void
strict_bool_controlling_expression(bool b, int i, double d, const void *p)
{
	if (__lint_false)	/* expect: 161 */
		do_nothing();	/* expect: statement not reached */

	if (__lint_true)	/* expect: 161 */
		do_nothing();

	if (b)
		do_nothing();

	if (/*CONSTCOND*/0)	/* expect: 333 */
		do_nothing();	/* expect: statement not reached [193] */

	if (/*CONSTCOND*/1)	/* expect: 333 */
		do_nothing();

	if (/*CONSTCOND*/2)	/* expect: 333 */
		do_nothing();

	/* Not allowed: There is no implicit conversion from scalar to bool. */
	if (i)			/* expect: 333 */
		do_nothing();
	if (i != 0)
		do_nothing();

	/* Not allowed: There is no implicit conversion from scalar to bool. */
	if (d)			/* expect: 333 */
		do_nothing();
	if (d != 0.0)
		do_nothing();

	/* Not allowed: There is no implicit conversion from scalar to bool. */
	if (p)			/* expect: 333 */
		do_nothing();
	if (p != (void *)0)
		do_nothing();
}

/*
 * strict-bool-operand-unary:
 *	Operator	bool?	scalar?
 *	!		yes	-
 *	&		yes	yes
 *	The other unary operators do not accept bool operands.
 */

void
strict_bool_operand_unary_not(void)
{
	bool b = __lint_false;

	b = !b;
	b = !!!b;
	b = !__lint_false;	/* expect: 161 *//* expect: 239 */
	b = !__lint_true;	/* expect: 161 *//* expect: 239 */

	int i = 0;

	i = !i;			/* expect: 330 */
	i = !!!i;		/* expect: 330 */
	i = !0;			/* expect: 330 */
	i = !1;			/* expect: 330 */
}

void
strict_bool_operand_unary_address(void)
{
	bool b = __lint_false;

	/* Taking the address of a bool lvalue. */
	bool *bp;
	bp = &b;
	*bp = b;
	b = *bp;
}

/* see strict_bool_operand_unary_all below for the other unary operators. */

/*
 * strict-bool-operand-binary:
 *	Operator	left:	bool?	other?	right:	bool?	other?
 *	.			-	yes		yes	yes
 *	->			-	yes		yes	yes
 *	<=, <, >=, >		-	yes		-	yes
 *	==, !=			yes	yes		yes	yes
 *	&			yes	yes		yes	yes
 *	^			yes	yes		yes	yes
 *	|			yes	yes		yes	yes
 *	&&			yes	-		yes	-
 *	||			yes	-		yes	-
 *	?			yes	-		yes	yes
 *	:			yes	yes		yes	yes
 *	=			yes	yes		yes	yes
 *	&=, ^=, |=		yes	yes		yes	yes
 *	,			yes	yes		yes	yes
 *	The other binary operators do not accept bool operands.
 */

/*
 * Ensure that bool members can be accessed as usual.
 */
void
strict_bool_operand_binary_dot_arrow(void)
{
	struct bool_struct {
		bool b;
	};

	/* Initialize and assign using boolean constants. */
	bool b = __lint_false;
	b = __lint_true;

	/* Access a struct member using the '.' operator. */
	struct bool_struct bs = { __lint_true };
	b = bs.b;
	bs.b = b;
	bs.b = 0;		/* expect: 107 */

	/* Access a struct member using the '->' operator. */
	struct bool_struct *bsp = &bs;
	b = bsp->b;
	bsp->b = b;
	bsp->b = 0;		/* expect: 107 */
}

int
strict_bool_operand_binary(bool b, int i)
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
	b = i && i;		/* expect: 331 *//* expect: 332 */
	b = i || i;		/* expect: 331 *//* expect: 332 */

	b = b && 0;		/* expect: 332 */
	b = 0 && b;		/* expect: 331 */
	b = b || 0;		/* expect: 332 */
	b = 0 || b;		/* expect: 331 */

	return i;
}

void
strict_bool_operand_unary_all(bool b)
{
	b = !b;
	b = ~b;			/* expect: 335 */
	++b;			/* expect: 335 */
	--b;			/* expect: 335 */
	b++;			/* expect: 335 */
	b--;			/* expect: 335 */
	b = +b;			/* expect: 335 */
	b = -b;			/* expect: 335 */
}

void
strict_bool_operand_binary_all(bool b, unsigned u)
{
	b = b * b;		/* expect: 336 *//* expect: 337 */
	b = b / b;		/* expect: 336 *//* expect: 337 */
	b = b % b;		/* expect: 336 *//* expect: 337 */
	b = b + b;		/* expect: 336 *//* expect: 337 */
	b = b - b;		/* expect: 336 *//* expect: 337 */
	b = b << b;		/* expect: 336 *//* expect: 337 */
	b = b >> b;		/* expect: 336 *//* expect: 337 */

	b = b < b;		/* expect: 336 *//* expect: 337 */
	b = b <= b;		/* expect: 336 *//* expect: 337 */
	b = b > b;		/* expect: 336 *//* expect: 337 */
	b = b >= b;		/* expect: 336 *//* expect: 337 */
	b = b == b;
	b = b != b;

	b = b & b;
	b = b ^ b;
	b = b | b;
	b = b && b;
	b = b || b;
	b = b ? b : b;

	b = b;
	b *= b;			/* expect: 336 *//* expect: 337 */
	b /= b;			/* expect: 336 *//* expect: 337 */
	b %= b;			/* expect: 336 *//* expect: 337 */
	b += b;			/* expect: 336 *//* expect: 337 */
	b -= b;			/* expect: 336 *//* expect: 337 */
	b <<= b;		/* expect: 336 *//* expect: 337 */
	b >>= b;		/* expect: 336 *//* expect: 337 */
	b &= b;
	b ^= b;
	b |= b;

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
	u = b ? u : u;
	u = b ? b : u;		/* expect: 107 */
	u = b ? u : b;		/* expect: 107 */
}

bool
strict_bool_operand_binary_comma(bool b, int i)
{
	b = (b, !b);		/* expect: 129 */
	i = (i, i + 1);		/* expect: 129 */
	return b;
}

/*
 * strict-bool-operator-result:
 *	The result type of the operators '!', '<', '<=', '>', '>=',
 *	'==', '!=', '&&', '||' is _Bool instead of int.
 */

void
strict_bool_operator_result(bool b)
{
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


/*
 * strict-bool-bitwise-and:
 *	Expressions of the form "flags & FLAG" are compatible with _Bool if
 *	the left operand has enum type, the right operand is an integer
 *	constant and the resulting value is used in a context where it is
 *	implicitly and immediately compared to zero.
 *
 *	Note: Examples for such contexts are controlling expressions or the
 *	operands of the operators '!', '&&', '||'.
 *
 *	Note: Counterexamples for contexts are assignments to a bool variable.
 *
 *	Note: These rules ensure that conforming code can be compiled without
 *	change in behavior using old compilers that implement bool as an
 *	ordinary integer type, without the special rule C99 6.3.1.2.
 */

enum Flags {
	FLAG0 = 1 << 0,
	FLAG1 = 1 << 1,
	FLAG28 = 1 << 28
};

void
strict_bool_bitwise_and_enum(enum Flags flags) /* expect: 231 */
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

/*
 * Demonstrate idiomatic code to query flags from an enum bit set.
 *
 * In all the controlling expressions in this function, the result of the
 * operator '&' is compared against 0.  This makes this pattern work, no
 * matter whether the bits are in the low-value range or in the high-value
 * range (such as FLAG28, which has the value 1073741824, which is more than
 * what would fit into an unsigned char).  Even if an enum could be extended
 * to larger types than int, this pattern would work.
 */
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


void
strict_bool_operator_eq_bool_int(void)
{
	(void)(strict_bool_conversion_return_false() == 0); /* expect: 107 */
}

void
strict_bool_assign_bit_field_then_compare(void)
{
	struct s {
		bool flag: 1;
	};

	struct s s = { __lint_false };

	(void)((s.flag = s.flag) != __lint_false);	/* expect: 129 */
}

void
bool_as_array_index(bool cond)
{
	static const char *repr[] = { "no", "yes" };
	/*
	 * The '+' in the error message reveals that lint internally
	 * translates 'arr[ind]' to '*(arr + ind)' in an early stage of
	 * parsing.
	 */
	println(repr[cond]);		/* expect: 337 */
	println(cond ? "yes" : "no");
}

void
do_while_false(void)
{
	do {

	} while (__lint_false);
}

void
do_while_true(void)
{
	do {

	} while (__lint_true);	/* expect: 161 */
}

void
initialization(void)
{
	struct {
		_Bool b;
	} var[] = {
	    { __lint_false },
	    { __lint_true },
	    { 0 },		/* expect: 107 */
	    { 1 },		/* expect: 107 */
	};
}
