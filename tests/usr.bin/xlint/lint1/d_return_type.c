/*	$NetBSD: d_return_type.c,v 1.2 2021/01/31 14:39:31 rillig Exp $	*/
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
	return arg;
}
