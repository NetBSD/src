/*	$NetBSD: d_return_type.c,v 1.6 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "d_return_type.c"

/* lint1-extra-flags: -X 351 */

enum A {
	A
};

enum B {
	B
};

enum A
func(enum B arg)
{
	/* expect+1: warning: function has return type 'enum A' but returns 'enum B' [211] */
	return arg;
}
