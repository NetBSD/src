/*	$NetBSD: msg_189.c,v 1.4 2021/04/09 20:12:01 rillig Exp $	*/
# 3 "msg_189.c"

/* Test for message: assignment of struct/union illegal in traditional C [189] */

/* lint1-flags: -tw */

struct s {
	int member;
};

void
example()
{
	struct s a, b;

	a.member = 3;
	b = a;			/* message 189 is not triggered anymore */
	/* expect-1: 'b' set but not used in function 'example' */
}
