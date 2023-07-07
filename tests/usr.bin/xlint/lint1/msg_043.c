/*	$NetBSD: msg_043.c,v 1.5 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_043.c"

/* Test for message: redefinition of '%s' hides earlier one [43] */

/* lint1-extra-flags: -h -X 351 */

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
