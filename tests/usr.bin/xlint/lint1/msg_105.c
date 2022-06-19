/*	$NetBSD: msg_105.c,v 1.4 2022/06/19 12:14:34 rillig Exp $	*/
# 3 "msg_105.c"

/* Test for message: non-unique member requires struct/union %s [105] */

/* lint1-flags: -tw */

/*
 * In traditional C, the expression 'x->y' did not only allow struct or union
 * pointers for 'x', but in fact any scalar expression, which would then be
 * dereferenced as if it were a struct or union.
 *
 * This led to ambiguities if several structs had a member of the same name
 * but with different offsets.  In such a case, that member name could only
 * be used with one of its actual struct types.
 */

struct one {
	int member;
};

struct two {
	int before_member;	/* make the offset of 'member' different */
	int member;
};

struct three {
	int x;
	int y;
};

int
example(x)
	int *x;
{
	/* expect+1: error: non-unique member requires struct/union pointer [105] */
	return x->member;
}

int
member_of_wrong_struct(t)
	struct three *t;
{
	/* expect+1: error: illegal use of member 'member' [102] */
	return t->member;
}
