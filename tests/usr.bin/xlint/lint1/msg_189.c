/*	$NetBSD: msg_189.c,v 1.2 2021/01/30 17:56:29 rillig Exp $	*/
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
}
