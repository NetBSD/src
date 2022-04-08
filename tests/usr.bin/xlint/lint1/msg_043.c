/*	$NetBSD: msg_043.c,v 1.3 2022/04/08 21:29:29 rillig Exp $	*/
# 3 "msg_043.c"

/* Test for message: redefinition hides earlier one: %s [43] */

/* lint1-extra-flags: -h */

struct s {
	int member;
};

void
example(void)
{
	/* expect+1: warning: redefinition hides earlier one: s [43] */
	struct s {
		int member;
	};
}
