/*	$NetBSD: d_return_type.c,v 1.3 2021/02/21 09:07:58 rillig Exp $	*/
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
	return arg;		/* expect: 211 */
}
