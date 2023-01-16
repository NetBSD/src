/*	$NetBSD: expr_sizeof.c,v 1.4 2023/01/16 00:37:59 rillig Exp $	*/
# 3 "expr_sizeof.c"

/*
 * C99 6.5.3.4 "The sizeof operator"
 * C11 6.5.3.4 "The sizeof operator"
 */

/*
 * A sizeof expression can either take a type name or an expression.
 */
void sink(unsigned long);

struct {
	int member;
} s, *ps;

/*
 * In a sizeof expression taking a type name, the type name must be enclosed
 * in parentheses.
 */
/* expect+1: error: negative array dimension (-4) [20] */
typedef int sizeof_int[-(int)sizeof(int)];

/*
 * In a sizeof expression taking an expression, the expression may or may not
 * be enclosed in parentheses, like any other expression.
 */
/* expect+1: error: negative array dimension (-4) [20] */
typedef int sizeof_paren_zero[-(int)sizeof(0)];
/* expect+1: error: negative array dimension (-4) [20] */
typedef int sizeof_zero[-(int)sizeof 0];

/*
 * Even though 's' is not a constant expression, 'sizeof s' is.
 */
/* expect+1: error: negative array dimension (-4) [20] */
typedef int sizeof_global_var[-(int)sizeof s];
/* expect+1: error: negative array dimension (-4) [20] */
typedef int sizeof_paren_global_var[-(int)sizeof(s)];

/*
 * Even though 'sizeof(s)' may look like a function call expression, the
 * parentheses around 's' are ordinary parentheses and do not influence the
 * precedence.
 *
 * Therefore, the '.' following the '(s)' takes precedence over the 'sizeof'.
 * Same for the '->' following the '(ps)'.  Same for the '[0]' following the
 * '(arr)'.
 */
/* expect+1: error: negative array dimension (-4) [20] */
typedef int sizeof_paren_global_struct_member[-(int)sizeof(s).member];
/* expect+1: error: negative array dimension (-4) [20] */
typedef int sizeof_paren_global_ptr_struct_member[-(int)sizeof(ps)->member];
int arr[] = { 1, 2, 3 };
/* expect+1: error: negative array dimension (-3) [20] */
typedef int arr_count[-(int)sizeof(arr) / (int)sizeof(arr)[0]];

/* FIXME: 'n' is actually used, for the variable length array. */
/* expect+2: warning: argument 'n' unused in function 'variable_length_array' [231] */
void
variable_length_array(int n)
{
	int local_arr[n + 5];

	/*
	 * Since the array length is not constant, it cannot be used in a
	 * typedef.  Code like this is already rejected by the compiler.  For
	 * simplicity, lint assumes that the array has length 1.
	 */
	/* expect+1: error: negative array dimension (-4) [20] */
	typedef int sizeof_local_arr[-(int)sizeof(local_arr)];
}
