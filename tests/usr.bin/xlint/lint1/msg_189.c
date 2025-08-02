/*	$NetBSD: msg_189.c,v 1.6.4.1 2025/08/02 05:58:17 perseant Exp $	*/
# 3 "msg_189.c"

/* Test for message: assignment of struct/union invalid in traditional C [189] */
/* This message is not used. */

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
	/* expect-1: warning: 'b' set but not used in function 'example' [191] */
}
