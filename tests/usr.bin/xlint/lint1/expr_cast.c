/*	$NetBSD: expr_cast.c,v 1.1 2021/08/03 18:03:54 rillig Exp $	*/
# 3 "expr_cast.c"

/*
 * Tests for value conversion using a cast-expression.
 *
 * K&R C does not mention any restrictions on the target type.
 * C90 requires both the source type and the target type to be scalar.
 */

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
	/* expect-3: warning: 'local' set but not used in function 'cast' [191] */
	/*
	 * ^^ XXX: The variable _is_ used, but only in a semantically wrong
	 * expression.  Lint should rather warn about the invalid cast in the
	 * 'return' statement, but since all C compilers since C90 are
	 * required to detect this already, there is no point in duplicating
	 * that work.
	 */

	return (struct S)local;
}
