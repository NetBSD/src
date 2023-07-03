/*	$NetBSD: d_c99_bool.c,v 1.11 2023/07/03 09:37:14 rillig Exp $	*/
# 3 "d_c99_bool.c"

/*
 * C99 6.3.1.2 says: "When any scalar value is converted to _Bool, the result
 * is 0 if the value compares equal to 0; otherwise the result is 1."
 *
 * This is different from the other integer types, which get truncated or
 * invoke undefined behavior.
 */

/* lint1-extra-flags: -X 351 */

/* expect+1: error: negative array dimension (-2) [20] */
int int_0[(_Bool)0 ? -1 : -2];

/* expect+1: error: negative array dimension (-1) [20] */
int int_1[(_Bool)1 ? -1 : -2];

/* expect+1: error: negative array dimension (-1) [20] */
int int_2[(_Bool)2 ? -1 : -2];

/* expect+1: error: negative array dimension (-1) [20] */
int int_256[(_Bool)256 ? -1 : -2];

/* expect+1: error: negative array dimension (-2) [20] */
int null_pointer[(_Bool)(void *)0 ? -1 : -2];

/*
 * XXX: In initializers for global variables, taking the address of a variable
 * is allowed and may be modified by a constant offset.  This is not a constant
 * expression though.
 *
 * In such a case, the grammar rule array_size_opt calls to_int_constant, which
 * returns 1 for the array size without reporting an error.  This is why
 * neither of the following array declarations generates an error message.
 */
char ch;
int nonnull_pointer_converts_to_false[(_Bool)&ch ? -1 : 1];
int nonnull_pointer_converts_to_true_[(_Bool)&ch ? 1 : -1];

/* expect+1: error: negative array dimension (-1) [20] */
int double_minus_1_0[(_Bool)-1.0 ? -1 : -2];

/* expect+1: error: negative array dimension (-1) [20] */
int double_minus_0_5[(_Bool)-0.5 ? -1 : -2];

/* expect+1: error: negative array dimension (-2) [20] */
int double_minus_0_0[(_Bool)-0.0 ? -1 : -2];

/* expect+1: error: negative array dimension (-2) [20] */
int double_0_0[(_Bool)0.0 ? -1 : -2];

/* The C99 rationale explains in 6.3.1.2 why (_Bool)0.5 is true. */
/* expect+1: error: negative array dimension (-1) [20] */
int double_0_5_converts_to_false[(_Bool)0.5 ? -1 : -2];

/* expect+1: error: negative array dimension (-1) [20] */
int double_1_0_converts_to_false[(_Bool)1.0 ? -1 : -2];

_Bool
convert_to_bool(int selector)
{
	static struct variant {
		_Bool b;
		char c;
		int i;
		double d;
		enum color {
			RED
		} e;
		const char *pcc;
		void (*f)(void);
		double _Complex dc;
	} v = { .i = 0 };

	switch (selector) {
	case 0: return v.b;
	case 1: return v.c;
	case 2: return v.i;
	case 3: return v.d;
	case 4: return v.e;
	case 5: return v.pcc;
	case 6: return v.f;
	case 7: return v.dc;
	default: return v.b;
	}
}
