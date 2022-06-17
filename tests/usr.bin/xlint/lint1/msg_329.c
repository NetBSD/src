/*	$NetBSD: msg_329.c,v 1.4 2022/06/17 06:59:16 rillig Exp $	*/
# 3 "msg_329.c"

// Test for message: type '%s' is not a member of '%s' [329]

union u {
	int i1;
	int i2;
	void *vp;
};

void
example(void)
{
	/*
	 * A type cast to a union type is valid if the source type is any
	 * member type of the union.  Since all union members with the same
	 * type have the same representation, the name of the union member
	 * doesn't matter.
	 *
	 * XXX: could there be padding bits or other tricky details that are
	 * settable per-member?  These could make the type alone insufficient
	 * for determining the exact representation.
	 *
	 * C99 6.5.4 "Cast operators" does not mention a union cast.  On the
	 * contrary, it says that the type name shall specify a scalar type.
	 *
	 * C11 6.5.4 "Cast operators" differs from C99 but still requires
	 * scalar types for both the target type and the source value.
	 *
	 * This is a GCC extension.
	 * See https://gcc.gnu.org/onlinedocs/gcc/Cast-to-Union.html.
	 */
	union u u_i1 = (union u)3;
	union u u_vp = (union u)(void *)0;
	/* expect+1: error: type 'pointer to char' is not a member of 'union u' [329] */
	union u u_cp = (union u)(char *)0;
}
