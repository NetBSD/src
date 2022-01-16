/*	$NetBSD: d_c99_bool.c,v 1.9 2022/01/16 08:40:31 rillig Exp $	*/
# 3 "d_c99_bool.c"

/*
 * C99 6.3.1.2 says: "When any scalar value is converted to _Bool, the result
 * is 0 if the value compares equal to 0; otherwise the result is 1."
 *
 * This is different from the other integer types, which get truncated or
 * invoke undefined behavior.
 */

/* Below, each false statement produces "negative array dimension" [20]. */

int int_0_converts_to_false[(_Bool)0 ? -1 : 1];
/* expect+1: error: negative array dimension (-1) [20] */
int int_0_converts_to_true_[(_Bool)0 ? 1 : -1];

/* expect+1: error: negative array dimension (-1) [20] */
int int_1_converts_to_false[(_Bool)1 ? -1 : 1];
int int_1_converts_to_true_[(_Bool)1 ? 1 : -1];

/* expect+1: error: negative array dimension (-1) [20] */
int int_2_converts_to_false[(_Bool)2 ? -1 : 1];
int int_2_converts_to_true_[(_Bool)2 ? 1 : -1];

/* expect+1: error: negative array dimension (-1) [20] */
int int_256_converts_to_false[(_Bool)256 ? -1 : 1];
int int_256_converts_to_true_[(_Bool)256 ? 1 : -1];

int null_pointer_converts_to_false[(_Bool)(void *)0 ? -1 : 1];
/* expect+1: error: negative array dimension (-1) [20] */
int null_pointer_converts_to_true_[(_Bool)(void *)0 ? 1 : -1];

/*
 * XXX: lint does not treat the address of a global variable as a constant
 * expression.  This goes against C99 6.6p7 but is probably not too relevant
 * in practice.
 *
 * In such a case, to_int_constant(tn, false) in cgram.y:array_size_opt
 * returns 1 for the array size.  This is why neither of the following array
 * declarations generates an error message.
 */
char ch;
int nonnull_pointer_converts_to_false[(_Bool)&ch ? -1 : 1];
int nonnull_pointer_converts_to_true_[(_Bool)&ch ? 1 : -1];

/* expect+1: error: negative array dimension (-1) [20] */
int double_minus_1_0_converts_to_false[(_Bool)-1.0 ? -1 : 1];
int double_minus_1_0_converts_to_true_[(_Bool)-1.0 ? 1 : -1];

/* expect+1: error: negative array dimension (-1) [20] */
int double_minus_0_5_converts_to_false[(_Bool)-0.5 ? -1 : 1];
int double_minus_0_5_converts_to_true_[(_Bool)-0.5 ? 1 : -1];

int double_minus_0_0_converts_to_false[(_Bool)-0.0 ? -1 : 1];
/* expect+1: error: negative array dimension (-1) [20] */
int double_minus_0_0_converts_to_true_[(_Bool)-0.0 ? 1 : -1];

int double_0_0_converts_to_false[(_Bool)0.0 ? -1 : 1];
/* expect+1: error: negative array dimension (-1) [20] */
int double_0_0_converts_to_true_[(_Bool)0.0 ? 1 : -1];

/* The C99 rationale explains in 6.3.1.2 why (_Bool)0.5 is true. */
/* expect+1: error: negative array dimension (-1) [20] */
int double_0_5_converts_to_false[(_Bool)0.5 ? -1 : 1];
int double_0_5_converts_to_true_[(_Bool)0.5 ? 1 : -1];

/* expect+1: error: negative array dimension (-1) [20] */
int double_1_0_converts_to_false[(_Bool)1.0 ? -1 : 1];
int double_1_0_converts_to_true_[(_Bool)1.0 ? 1 : -1];

_Bool
bool_to_bool(_Bool b)
{
	return b;
}

_Bool
char_to_bool(char c)
{
	return c;
}

_Bool
int_to_bool(int i)
{
	return i;
}

_Bool
double_to_bool(double d)
{
	return d;
}

enum color {
	RED
};

_Bool
enum_to_bool(enum color e)
{
	return e;
}

_Bool
pointer_to_bool(const char *p)
{
	return p;
}

_Bool
function_pointer_to_bool(void (*f)(void))
{
	return f;
}

_Bool
complex_to_bool(double _Complex c)
{
	return c;
}
