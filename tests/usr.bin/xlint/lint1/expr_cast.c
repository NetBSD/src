/*	$NetBSD: expr_cast.c,v 1.6 2024/06/08 13:50:47 rillig Exp $	*/
# 3 "expr_cast.c"

/*
 * Tests for value conversion using a cast-expression.
 *
 * K&R C does not mention any restrictions on the target type.
 * C90 requires both the source type and the target type to be scalar.
 *
 * GCC allows casting to a struct type but there is no documentation about
 * it at https://gcc.gnu.org/onlinedocs/gcc/C-Extensions.html.  See
 * c-typeck.c, function build_c_cast, RECORD_OR_UNION_TYPE_P.
 */

/* lint1-flags: -Sw -X 351 */

struct S {
	int member;
};

struct S
cast(void)
{
	struct S {
		double incompatible;
	} local = {
		0.0
	};

	/* expect+2: error: invalid cast from 'struct S' to 'struct S' [147] */
	/* expect+1: error: function 'cast' expects to return value [214] */
	return (struct S)local;
}

/*
 * https://gnats.netbsd.org/22119
 *
 * Before 2021-02-28, lint crashed in cast() since the target type of the
 * cast is NULL.
*/
void
cast_from_error(void)
{
	void (*f1)(void);

	/* expect+1: error: 'p' undefined [99] */
	f1 = (void (*)(void))p;
	/* expect+2: error: function returns illegal type 'function(void) returning pointer to void' [15] */
	/* expect+1: error: invalid cast from 'int' to 'function() returning pointer to function(void) returning pointer to void' [147] */
	f1 = (void *()(void))p;		/* crash before 2021-02-28 */
}

/*
 * Pointer casts had been valid lvalues in GCC before 4.0.
 *
 * https://gcc.gnu.org/onlinedocs/gcc-3.4.6/gcc/Lvalues.html#Lvalues
 *
 * C99 6.5.4 "Cast operators" footnote 85 says "A cast does not yield an
 * lvalue".
 */

void take_ptr(const void *);

void *
lvalue_cast(void *p)
{
	struct str {
		int member;
	};

	/* expect+2: error: a cast does not yield an lvalue [163] */
	/* expect+1: error: left operand of '=' must be lvalue [114] */
	((struct str *)p) = 0;

	/* expect+2: error: a cast does not yield an lvalue [163] */
	/* expect+1: error: operand of '&' must be lvalue [114] */
	take_ptr(&(const void *)p);

	return p;
}
