/*	$NetBSD: decl_enum.c,v 1.2 2022/04/08 21:29:29 rillig Exp $	*/
# 3 "decl_enum.c"

/*
 * Tests for enum declarations.
 */

/* cover 'enumerator_list: error' */
enum {
	/* expect+1: error: syntax error 'goto' [249] */
	goto
};

/* cover 'enum_specifier: enum error' */
/* expect+1: error: syntax error 'goto' [249] */
enum goto {
	A
};
/* expect-1: warning: empty declaration [0] */


/*
 * Ensure that nested enum declarations get the value of each enum constant
 * right.  The variable containing the "current enum value" does not account
 * for these nested declarations.  Such declarations don't occur in practice
 * though.
 */
enum outer {
	o1 = sizeof(
	    enum inner {
		    i1 = 10000, i2, i3
	    }
	),
	/*
	 * The only attribute that GCC 12 allows for enum constants is
	 * __deprecated__, and there is no way to smuggle an integer constant
	 * expression into the attribute.  If there were a way, and the
	 * expression contained an enum declaration, the value of the outer
	 * enum constant would become the value of the last seen inner enum
	 * constant.  This is because 'enumval' is a simple scalar variable,
	 * not a stack.  If it should ever become necessary to account for
	 * nested enum declarations, a field should be added in dinfo_t.
	 */
	o2 __attribute__((__deprecated__)),
	o3 = i3
};

/* expect+1: error: negative array dimension (-10000) [20] */
typedef int reveal_i1[-i1];
/* expect+1: error: negative array dimension (-10001) [20] */
typedef int reveal_i2[-i2];
/* expect+1: error: negative array dimension (-10002) [20] */
typedef int reveal_i3[-i3];

/* expect+1: error: negative array dimension (-4) [20] */
typedef int reveal_o1[-o1];
/* expect+1: error: negative array dimension (-5) [20] */
typedef int reveal_o2[-o2];
/* expect+1: error: negative array dimension (-10002) [20] */
typedef int reveal_o3[-o3];
