/*	$NetBSD: d_return_type.c,v 1.4 2022/01/15 14:22:03 rillig Exp $	*/
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
	/* expect+1: warning: return value type mismatch (enum A) and (enum B) [211] */
	return arg;
}
