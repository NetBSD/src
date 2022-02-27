/*	$NetBSD: d_c99_bool_strict.c,v 1.37 2022/02/27 08:31:26 rillig Exp $	*/
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
	/* expect+1: error: argument #1 expects '_Bool', gets passed 'int' [334] */
	accept_bool(0);
	/* expect+1: error: argument #1 expects '_Bool', gets passed 'int' [334] */
	accept_bool(1);
	/* expect+1: error: argument #1 expects '_Bool', gets passed 'int' [334] */
	accept_bool(2);
}

enum strict_bool_constant_expressions {
	/* Ok: __lint_false is a boolean constant expression. */
	/* expect+1: warning: constant in conditional context [161] */
	FALSE = __lint_false ? 100 : 101,

	/* Ok: __lint_true is a boolean constant expression. */
	/* expect+1: warning: constant in conditional context [161] */
	TRUE = __lint_true ? 100 : 101,

	/* Not ok: an integer is not a boolean constant expression. */
	/* expect+1: error: left operand of '?' must be bool, not 'int' [331] */
	INT0 = 0 ? 100 : 101,

	/* Not ok: an integer is not a boolean constant expression. */
	/* expect+1: error: left operand of '?' must be bool, not 'int' [331] */
	INT1 = 1 ? 100 : 101,

	/* Not ok: 2 is not a boolean constant. */
	/* expect+1: error: left operand of '?' must be bool, not 'int' [331] */
	INT2 = 2 ? 100 : 101,

	/* Not ok: compound integer expressions are not bool. */
	/* expect+1: error: left operand of '?' must be bool, not 'int' [331] */
	ARITH = (2 - 2) ? 100 : 101,

	/*
	 * Without strict bool mode, these two variants of an expression can
	 * occur when a preprocessor macro is either defined to 1 or left
	 * empty (since C99), as in lint1/ops.def.
	 *
	 * In strict bool mode, the resulting expression can be compared
	 * against 0 to achieve the same effect (so +0 != 0 or 1 + 0 != 0).
	 */
	/* expect+1: error: left operand of '?' must be bool, not 'int' [331] */
	BINARY_PLUS = (1 + 0) ? 100 : 101,
	/* expect+1: error: left operand of '?' must be bool, not 'int' [331] */
	UNARY_PLUS = (+0) ? 100 : 101,

	/* The main operator '>' has return type bool. */
	/* expect+1: warning: constant in conditional context [161] */
	Q1 = (13 > 12) ? 100 : 101,

	/*
	 * The parenthesized expression has type int and thus cannot be
	 * used as the controlling expression in the '?:' operator.
	 */
	/* expect+2: warning: constant in conditional context [161] */
	/* expect+1: error: left operand of '?' must be bool, not 'int' [331] */
	Q2 = (13 > 12 ? 1 : 7) ? 100 : 101,

	/* expect+1: error: integral constant expression expected [55] */
	BINAND_BOOL = __lint_false & __lint_true,
	BINAND_INT = 0 & 1,

	/* expect+1: error: integral constant expression expected [55] */
	BINXOR_BOOL = __lint_false ^ __lint_true,
	BINXOR_INT = 0 ^ 1,

	/* expect+1: error: integral constant expression expected [55] */
	BINOR_BOOL = __lint_false | __lint_true,
	BINOR_INT = 0 | 1,

	/* expect+2: warning: constant in conditional context [161] */
	/* expect+1: error: integral constant expression expected [55] */
	LOGOR_BOOL = __lint_false || __lint_true,
	/* expect+2: error: left operand of '||' must be bool, not 'int' [331] */
	/* expect+1: error: right operand of '||' must be bool, not 'int' [332] */
	LOGOR_INT = 0 || 1,

	/* expect+2: warning: constant in conditional context [161] */
	/* expect+1: error: integral constant expression expected [55] */
	LOGAND_BOOL = __lint_false && __lint_true,
	/* expect+2: error: left operand of '&&' must be bool, not 'int' [331] */
	/* expect+1: error: right operand of '&&' must be bool, not 'int' [332] */
	LOGAND_INT = 0 && 1,
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
	/* expect+1: error: operands of '=' have incompatible types (_Bool != unsigned int) [107] */
	b = flags.uint_flag;
	flags.bool_flag = b;
	/* expect+1: error: operands of '=' have incompatible types (unsigned int != _Bool) [107] */
	flags.uint_flag = b;

	b = flags_ptr->bool_flag;
	/* expect+1: error: operands of '=' have incompatible types (_Bool != unsigned int) [107] */
	b = flags_ptr->uint_flag;
	flags_ptr->bool_flag = b;
	/* expect+1: error: operands of '=' have incompatible types (unsigned int != _Bool) [107] */
	flags_ptr->uint_flag = b;
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
	/* expect+1: error: return value type mismatch (_Bool) and (int) [211] */
	return 0;
}

bool
strict_bool_conversion_return_1(void)
{
	/* expect+1: error: return value type mismatch (_Bool) and (int) [211] */
	return 1;
}

bool
strict_bool_conversion_return_2(void)
{
	/* expect+1: error: return value type mismatch (_Bool) and (int) [211] */
	return 2;
}

/* expect+2: warning: argument 'p' unused in function 'strict_bool_conversion_return_pointer' [231] */
bool
strict_bool_conversion_return_pointer(const void *p)
{
	/* expect+1: error: return value type mismatch (_Bool) and (pointer) [211] */
	return p;
}

char
strict_bool_conversion_return_false_as_char(void)
{
	/* expect+1: error: return value type mismatch (char) and (_Bool) [211] */
	return __lint_false;
}

char
strict_bool_conversion_return_true_as_char(void)
{
	/* expect+1: error: return value type mismatch (char) and (_Bool) [211] */
	return __lint_true;
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
	/* expect+2: error: argument #2 expects 'int', gets passed '_Bool' [334] */
	/* expect+1: error: argument #3 expects 'pointer', gets passed '_Bool' [334] */
	take_arguments(b, b, b);

	/* Implicitly converting int to bool (arg #1). */
	/* expect+2: error: argument #1 expects '_Bool', gets passed 'int' [334] */
	/* expect+1: warning: illegal combination of pointer (pointer to const char) and integer (int), arg #3 [154] */
	take_arguments(i, i, i);

	/* Implicitly converting pointer to bool (arg #1). */
	/* expect+2: error: argument #1 expects '_Bool', gets passed 'pointer' [334] */
	/* expect+1: warning: illegal combination of integer (int) and pointer (pointer to const char), arg #2 [154] */
	take_arguments(p, p, p);

	/* Passing bool as vararg. */
	/* TODO: maybe expect+1: arg#4 should not be bool but scalar */
	take_arguments(b, i, p, b, i, p);

	/* Passing a bool constant. */
	take_arguments(__lint_false, i, p);

	/* Passing a bool constant. */
	take_arguments(__lint_true, i, p);

	/* Trying to pass integer constants. */
	/* expect+1: error: argument #1 expects '_Bool', gets passed 'int' [334] */
	take_arguments(0, i, p);
	/* expect+1: error: argument #1 expects '_Bool', gets passed 'int' [334] */
	take_arguments(1, i, p);
	/* expect+1: error: argument #1 expects '_Bool', gets passed 'int' [334] */
	take_arguments(2, i, p);
}

void
strict_bool_conversion_between_bool_and_int(void)
{
	bool b;
	int i;

	/* expect+1: error: operands of '=' have incompatible types (_Bool != int) [107] */
	b = 0;
	b = __lint_false;
	/* expect+1: error: operands of '=' have incompatible types (_Bool != int) [107] */
	b = 1;
	b = __lint_true;

	i = 0;
	/* expect+1: error: operands of '=' have incompatible types (int != _Bool) [107] */
	i = __lint_false;
	i = 1;
	/* expect+1: error: operands of '=' have incompatible types (int != _Bool) [107] */
	i = __lint_true;

	/* expect+1: error: operands of '=' have incompatible types (int != _Bool) [107] */
	i = b;
	/* expect+1: error: operands of '=' have incompatible types (_Bool != int) [107] */
	b = i;
}

/* expect+2: warning: argument 'b' unused in function 'strict_bool_conversion_from_bool_to_scalar' [231] */
void
strict_bool_conversion_from_bool_to_scalar(bool b)
{
	int i;
	unsigned u;
	double d;
	void *p;

	/* expect+1: error: operands of '=' have incompatible types (int != _Bool) [107] */
	i = b;
	/* expect+1: error: operands of '=' have incompatible types (unsigned int != _Bool) [107] */
	u = b;
	/* expect+1: error: operands of '=' have incompatible types (double != _Bool) [107] */
	d = b;
	/* expect+1: error: operands of '=' have incompatible types (pointer != _Bool) [107] */
	p = b;
}

/*
 * strict-bool-controlling-expression:
 *	Controlling expressions in 'if', 'while', 'for', '?:' must be of
 *	type bool.
 */

void
strict_bool_controlling_expression(bool b, int i, double d, const void *p)
{
	/* expect+1: warning: constant in conditional context [161] */
	if (__lint_false)
		do_nothing();
	/* expect-1: warning: statement not reached [193] */

	/* expect+1: warning: constant in conditional context [161] */
	if (__lint_true)
		do_nothing();

	if (b)
		do_nothing();

	/* expect+1: error: controlling expression must be bool, not 'int' [333] */
	if (/*CONSTCOND*/0)
		do_nothing();
	/* expect-1: warning: statement not reached [193] */

	/* expect+1: error: controlling expression must be bool, not 'int' [333] */
	if (/*CONSTCOND*/1)
		do_nothing();

	/* expect+1: error: controlling expression must be bool, not 'int' [333] */
	if (/*CONSTCOND*/2)
		do_nothing();

	/* Not allowed: There is no implicit conversion from scalar to bool. */
	/* expect+1: error: controlling expression must be bool, not 'int' [333] */
	if (i)
		do_nothing();
	if (i != 0)
		do_nothing();

	/* Not allowed: There is no implicit conversion from scalar to bool. */
	/* expect+1: error: controlling expression must be bool, not 'double' [333] */
	if (d)
		do_nothing();
	if (d != 0.0)
		do_nothing();

	/* Not allowed: There is no implicit conversion from scalar to bool. */
	/* expect+1: error: controlling expression must be bool, not 'pointer' [333] */
	if (p)
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
	/* expect+2: warning: constant in conditional context [161] */
	/* expect+1: warning: constant argument to '!' [239] */
	b = !__lint_false;
	/* expect+2: warning: constant in conditional context [161] */
	/* expect+1: warning: constant argument to '!' [239] */
	b = !__lint_true;

	int i = 0;

	/* expect+1: error: operand of '!' must be bool, not 'int' [330] */
	i = !i;
	/* expect+1: error: operand of '!' must be bool, not 'int' [330] */
	i = !!!i;
	/* expect+1: error: operand of '!' must be bool, not 'int' [330] */
	i = !0;
	/* expect+1: error: operand of '!' must be bool, not 'int' [330] */
	i = !1;
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
	/* expect+1: error: operands of '=' have incompatible types (_Bool != int) [107] */
	bs.b = 0;

	/* Access a struct member using the '->' operator. */
	struct bool_struct *bsp = &bs;
	b = bsp->b;
	bsp->b = b;
	/* expect+1: error: operands of '=' have incompatible types (_Bool != int) [107] */
	bsp->b = 0;
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
	/* expect+1: error: operand of '!' must be bool, not 'int' [330] */
	b = !i;
	/* expect+2: error: left operand of '&&' must be bool, not 'int' [331] */
	/* expect+1: error: right operand of '&&' must be bool, not 'int' [332] */
	b = i && i;
	/* expect+2: error: left operand of '||' must be bool, not 'int' [331] */
	/* expect+1: error: right operand of '||' must be bool, not 'int' [332] */
	b = i || i;

	/* expect+1: error: right operand of '&&' must be bool, not 'int' [332] */
	b = b && 0;
	/* expect+1: error: left operand of '&&' must be bool, not 'int' [331] */
	b = 0 && b;
	/* expect+1: error: right operand of '||' must be bool, not 'int' [332] */
	b = b || 0;
	/* expect+1: error: left operand of '||' must be bool, not 'int' [331] */
	b = 0 || b;

	return i;
}

void
strict_bool_operand_unary_all(bool b)
{
	b = !b;
	/* expect+1: error: operand of '~' must not be bool [335] */
	b = ~b;
	/* expect+1: error: operand of '++x' must not be bool [335] */
	++b;
	/* expect+1: error: operand of '--x' must not be bool [335] */
	--b;
	/* expect+1: error: operand of 'x++' must not be bool [335] */
	b++;
	/* expect+1: error: operand of 'x--' must not be bool [335] */
	b--;
	/* expect+1: error: operand of '+' must not be bool [335] */
	b = +b;
	/* expect+1: error: operand of '-' must not be bool [335] */
	b = -b;
}

void
strict_bool_operand_binary_all(bool b, unsigned u)
{
	/* expect+2: error: left operand of '*' must not be bool [336] */
	/* expect+1: error: right operand of '*' must not be bool [337] */
	b = b * b;
	/* expect+2: error: left operand of '/' must not be bool [336] */
	/* expect+1: error: right operand of '/' must not be bool [337] */
	b = b / b;
	/* expect+2: error: left operand of '%' must not be bool [336] */
	/* expect+1: error: right operand of '%' must not be bool [337] */
	b = b % b;
	/* expect+2: error: left operand of '+' must not be bool [336] */
	/* expect+1: error: right operand of '+' must not be bool [337] */
	b = b + b;
	/* expect+2: error: left operand of '-' must not be bool [336] */
	/* expect+1: error: right operand of '-' must not be bool [337] */
	b = b - b;
	/* expect+2: error: left operand of '<<' must not be bool [336] */
	/* expect+1: error: right operand of '<<' must not be bool [337] */
	b = b << b;
	/* expect+2: error: left operand of '>>' must not be bool [336] */
	/* expect+1: error: right operand of '>>' must not be bool [337] */
	b = b >> b;

	/* expect+2: error: left operand of '<' must not be bool [336] */
	/* expect+1: error: right operand of '<' must not be bool [337] */
	b = b < b;
	/* expect+2: error: left operand of '<=' must not be bool [336] */
	/* expect+1: error: right operand of '<=' must not be bool [337] */
	b = b <= b;
	/* expect+2: error: left operand of '>' must not be bool [336] */
	/* expect+1: error: right operand of '>' must not be bool [337] */
	b = b > b;
	/* expect+2: error: left operand of '>=' must not be bool [336] */
	/* expect+1: error: right operand of '>=' must not be bool [337] */
	b = b >= b;
	b = b == b;
	b = b != b;

	b = b & b;
	b = b ^ b;
	b = b | b;
	b = b && b;
	b = b || b;
	b = b ? b : b;

	b = b;
	/* expect+2: error: left operand of '*=' must not be bool [336] */
	/* expect+1: error: right operand of '*=' must not be bool [337] */
	b *= b;
	/* expect+2: error: left operand of '/=' must not be bool [336] */
	/* expect+1: error: right operand of '/=' must not be bool [337] */
	b /= b;
	/* expect+2: error: left operand of '%=' must not be bool [336] */
	/* expect+1: error: right operand of '%=' must not be bool [337] */
	b %= b;
	/* expect+2: error: left operand of '+=' must not be bool [336] */
	/* expect+1: error: right operand of '+=' must not be bool [337] */
	b += b;
	/* expect+2: error: left operand of '-=' must not be bool [336] */
	/* expect+1: error: right operand of '-=' must not be bool [337] */
	b -= b;
	/* expect+2: error: left operand of '<<=' must not be bool [336] */
	/* expect+1: error: right operand of '<<=' must not be bool [337] */
	b <<= b;
	/* expect+2: error: left operand of '>>=' must not be bool [336] */
	/* expect+1: error: right operand of '>>=' must not be bool [337] */
	b >>= b;
	b &= b;
	b ^= b;
	b |= b;

	/* Operations with mixed types. */
	/* expect+1: error: left operand of '*' must not be bool [336] */
	u = b * u;
	/* expect+1: error: right operand of '*' must not be bool [337] */
	u = u * b;
	/* expect+1: error: left operand of '/' must not be bool [336] */
	u = b / u;
	/* expect+1: error: right operand of '/' must not be bool [337] */
	u = u / b;
	/* expect+1: error: left operand of '%' must not be bool [336] */
	u = b % u;
	/* expect+1: error: right operand of '%' must not be bool [337] */
	u = u % b;
	/* expect+1: error: left operand of '+' must not be bool [336] */
	u = b + u;
	/* expect+1: error: right operand of '+' must not be bool [337] */
	u = u + b;
	/* expect+1: error: left operand of '-' must not be bool [336] */
	u = b - u;
	/* expect+1: error: right operand of '-' must not be bool [337] */
	u = u - b;
	/* expect+1: error: left operand of '<<' must not be bool [336] */
	u = b << u;
	/* expect+1: error: right operand of '<<' must not be bool [337] */
	u = u << b;
	/* expect+1: error: left operand of '>>' must not be bool [336] */
	u = b >> u;
	/* expect+1: error: right operand of '>>' must not be bool [337] */
	u = u >> b;
	u = b ? u : u;
	/* expect+1: error: operands of ':' have incompatible types (_Bool != unsigned int) [107] */
	u = b ? b : u;
	/* expect+1: error: operands of ':' have incompatible types (unsigned int != _Bool) [107] */
	u = b ? u : b;
}

bool
strict_bool_operand_binary_comma(bool b, int i)
{
	/* expect+1: warning: expression has null effect [129] */
	b = (b, !b);
	/* expect+1: warning: expression has null effect [129] */
	i = (i, i + 1);
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
	/* expect+1: error: operands of 'init' have incompatible types (char != _Bool) [107] */
	char c = b;
	/* expect+1: error: operands of 'init' have incompatible types (int != _Bool) [107] */
	int i = b;
	/* expect+1: error: operands of 'init' have incompatible types (double != _Bool) [107] */
	double d = b;
	/* expect+1: error: operands of 'init' have incompatible types (pointer != _Bool) [107] */
	void *p = b;

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
	/* expect+1: error: operands of '=' have incompatible types (int != _Bool) [107] */
	i = !b;
	/* expect+1: error: operands of '=' have incompatible types (int != _Bool) [107] */
	i = i == i;
	/* expect+1: error: operands of '=' have incompatible types (int != _Bool) [107] */
	i = i != i;
	/* expect+1: error: operands of '=' have incompatible types (int != _Bool) [107] */
	i = i < i;
	/* expect+1: error: operands of '=' have incompatible types (int != _Bool) [107] */
	i = i <= i;
	/* expect+1: error: operands of '=' have incompatible types (int != _Bool) [107] */
	i = i >= i;
	/* expect+1: error: operands of '=' have incompatible types (int != _Bool) [107] */
	i = i > i;
	/* expect+1: error: operands of '=' have incompatible types (int != _Bool) [107] */
	i = b && b;
	/* expect+1: error: operands of '=' have incompatible types (int != _Bool) [107] */
	i = b || b;
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

/* expect+2: warning: argument 'flags' unused in function 'strict_bool_bitwise_and_enum' [231] */
void
strict_bool_bitwise_and_enum(enum Flags flags)
{
	bool b;

	/*
	 * FLAG0 has the value 1 and thus can be stored in a bool variable
	 * without truncation.  Nevertheless this special case is not allowed
	 * because it would be too confusing if FLAG0 would work and all the
	 * other flags wouldn't.
	 */
	/* expect+1: error: operands of '=' have incompatible types (_Bool != int) [107] */
	b = flags & FLAG0;

	/*
	 * Assuming that FLAG1 is set in flags, a _Bool variable stores this
	 * as 1, as defined by C99 6.3.1.2.  A uint8_t variable would store
	 * it as 2, as that is the integer value of FLAG1.  Since FLAG1 fits
	 * in a uint8_t, no truncation takes place.
	 */
	/* expect+1: error: operands of '=' have incompatible types (_Bool != int) [107] */
	b = flags & FLAG1;

	/*
	 * In a _Bool variable, FLAG28 is stored as 1, since it is unequal to
	 * zero.  In a uint8_t, the stored value would be 0 since bit 28 is
	 * out of range for a uint8_t and thus gets truncated.
	 */
	/* expect+1: error: operands of '=' have incompatible types (_Bool != int) [107] */
	b = flags & FLAG28;
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
	/* expect+1: error: operands of '==' have incompatible types (_Bool != int) [107] */
	(void)(strict_bool_conversion_return_false() == 0);
}

void
strict_bool_assign_bit_field_then_compare(void)
{
	struct s {
		bool flag: 1;
	};

	struct s s = { __lint_false };

	/* expect+1: warning: expression has null effect [129] */
	(void)((s.flag = s.flag) != __lint_false);
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
	/* expect+1: error: right operand of '+' must not be bool [337] */
	println(repr[cond]);
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

	} while (__lint_true);
	/* expect-1: warning: constant in conditional context [161] */
}

void
initialization(void)
{
	struct {
		_Bool b;
	} var[] = {
	    { __lint_false },
	    { __lint_true },
	    /* expect+1: error: operands of 'init' have incompatible types (_Bool != int) [107] */
	    { 0 },
	    /* expect+1: error: operands of 'init' have incompatible types (_Bool != int) [107] */
	    { 1 },
	};
}

/*
 * For expressions that originate from a system header, the strict type rules
 * are relaxed a bit, to allow for expressions like 'flags & FLAG', even
 * though they are not strictly boolean.
 *
 * This shouldn't apply to function call expressions though since one of the
 * goals of strict bool mode is to normalize all expressions calling 'strcmp'
 * to be of the form 'strcmp(a, b) == 0' instead of '!strcmp(a, b)'.
 */
# 1 "stdio.h" 1 3 4
typedef struct stdio_file {
	int fd;
} FILE;
int ferror(FILE *);
FILE stdio_files[3];
FILE *stdio_stdout;
# 967 "d_c99_bool_strict.c" 2
# 1 "string.h" 1 3 4
int strcmp(const char *, const char *);
# 970 "d_c99_bool_strict.c" 2

void
controlling_expression(FILE *f, const char *a, const char *b)
{
	/* expect+1: error: controlling expression must be bool, not 'int' [333] */
	if (ferror(f))
		return;
	/* expect+1: error: controlling expression must be bool, not 'int' [333] */
	if (strcmp(a, b))
		return;
	/* expect+1: error: operand of '!' must be bool, not 'int' [330] */
	if (!ferror(f))
		return;
	/* expect+1: error: operand of '!' must be bool, not 'int' [330] */
	if (!strcmp(a, b))
		return;

	/*
	 * Before tree.c 1.395 from 2021-11-16, the expression below didn't
	 * produce a warning since the expression 'stdio_files' came from a
	 * system header (via a macro), and this property was passed up to
	 * the expression 'ferror(stdio_files[1])'.
	 *
	 * That was wrong though since the type of a function call expression
	 * only depends on the function itself but not its arguments types.
	 * The old rule had allowed a raw condition 'strcmp(a, b)' without
	 * the comparison '!= 0', as long as one of its arguments came from a
	 * system header.
	 *
	 * Seen in bin/echo/echo.c, function main, call to ferror.
	 */
	/* expect+5: error: controlling expression must be bool, not 'int' [333] */
	if (ferror(
# 1004 "d_c99_bool_strict.c" 3 4
	    &stdio_files[1]
# 1006 "d_c99_bool_strict.c"
	    ))
		return;

	/*
	 * Before cgram.y 1.369 from 2021-11-16, at the end of parsing the
	 * name 'stdio_stdout', the parser already looked ahead to the next
	 * token, to see whether it was the '(' of a function call.
	 *
	 * At that point, the parser was no longer in a system header,
	 * therefore 'stdio_stdout' had tn_sys == false, and this information
	 * was pushed down to the whole function call expression (which was
	 * another bug that got fixed in tree.c 1.395 from 2021-11-16).
	 */
	/* expect+5: error: controlling expression must be bool, not 'int' [333] */
	if (ferror(
# 1022 "d_c99_bool_strict.c" 3 4
	    stdio_stdout
# 1024 "d_c99_bool_strict.c"
	    ))
		return;

	/*
	 * In this variant of the pattern, there is a token ')' after the
	 * name 'stdio_stdout', which even before tree.c 1.395 from
	 * 2021-11-16 had the effect that at the end of parsing the name, the
	 * parser was still in the system header, thus setting tn_sys (or
	 * rather tn_relaxed at that time) to true.
	 */
	/* expect+5: error: controlling expression must be bool, not 'int' [333] */
	if (ferror(
# 1037 "d_c99_bool_strict.c" 3 4
	    (stdio_stdout)
# 1039 "d_c99_bool_strict.c"
	    ))
		return;

	/*
	 * Before cgram.y 1.369 from 2021-11-16, the comment following
	 * 'stdio_stdout' did not prevent the search for '('.  At the point
	 * where build_name called expr_alloc_tnode, the parser was already
	 * in the main file again, thus treating 'stdio_stdout' as not coming
	 * from a system header.
	 *
	 * This has been fixed in tree.c 1.395 from 2021-11-16.  Before that,
	 * an expression had come from a system header if its operands came
	 * from a system header, but that was only close to the truth.  In a
	 * case where both operands come from a system header but the
	 * operator comes from the main translation unit, the main
	 * translation unit still has control over the whole expression.  So
	 * the correct approach is to focus on the operator, not the
	 * operands.  There are a few corner cases where the operator is
	 * invisible (for implicit conversions) or synthetic (for translating
	 * 'arr[index]' to '*(arr + index)', but these are handled as well.
	 */
	/* expect+5: error: controlling expression must be bool, not 'int' [333] */
	if (ferror(
# 1063 "d_c99_bool_strict.c" 3 4
	    stdio_stdout /* comment */
# 1065 "d_c99_bool_strict.c"
	    ))
		return;
}
