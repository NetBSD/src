/*	$NetBSD: msg_043.c,v 1.4 2022/06/20 21:13:36 rillig Exp $	*/
# 3 "msg_043.c"

/* Test for message: redefinition of '%s' hides earlier one [43] */

/* lint1-extra-flags: -h */

struct s {
	int member;
};

void
example(void)
{
	/* expect+1: warning: redefinition of 's' hides earlier one [43] */
	struct s {
		int member;
	};
}
