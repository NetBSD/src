/*	$NetBSD: msg_159.c,v 1.4 2022/07/06 21:13:13 rillig Exp $	*/
# 3 "msg_159.c"

// Test for message: assignment in conditional context [159]

/* lint1-extra-flags: -h */

const char *
example(int a, int b)
{

	if (a == b)
		return "comparison, not parenthesized";

	if ((a == b))
		return "comparison, parenthesized";

	if (
# 20 "msg_159.c" 3 4
	    (a == b)
# 22 "msg_159.c"
	    )
		return "comparison, parenthesized, from system header";

	/* expect+1: warning: assignment in conditional context [159] */
	if (a = b)
		return "assignment, not parenthesized";

	/*
	 * XXX: GCC has the convention that an assignment that is
	 * parenthesized is intended as an assignment.
	 */
	/* expect+1: warning: assignment in conditional context [159] */
	if ((a = b))
		return "assignment, parenthesized";

	if ((a = b) != 0)
		return "explicit comparison after assignment";

	return "other";
}
