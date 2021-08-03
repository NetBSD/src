/*	$NetBSD: expr_cast.c,v 1.3 2021/08/03 18:44:33 rillig Exp $	*/
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

/* lint1-flags: -Sw */

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
	/* expect+1: warning: function 'cast' expects to return value [214] */
	return (struct S)local;
}
