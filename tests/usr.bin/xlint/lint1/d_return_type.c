/*	$NetBSD: d_return_type.c,v 1.5 2022/06/22 19:23:18 rillig Exp $	*/
# 3 "d_return_type.c"

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
