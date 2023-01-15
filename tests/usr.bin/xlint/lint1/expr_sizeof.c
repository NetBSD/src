/*	$NetBSD: expr_sizeof.c,v 1.1 2023/01/15 00:53:19 rillig Exp $	*/
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
 * associativity.  Therefore, the '.' following the '(s)' takes precedence
 * over the 'sizeof'.  Same for the '->' following the '(ps)'.
 */
/* expect+1: error: negative array dimension (-4) [20] */
typedef int sizeof_paren_global_struct_member[-(int)sizeof(s).member];
/* expect+1: error: negative array dimension (-4) [20] */
typedef int sizeof_paren_global_ptr_struct_member[-(int)sizeof(ps)->member];
