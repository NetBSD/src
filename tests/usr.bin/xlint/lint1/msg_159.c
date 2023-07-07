/*	$NetBSD: msg_159.c,v 1.7 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_159.c"

// Test for message: assignment in conditional context [159]

/* lint1-extra-flags: -h -X 351 */

const char *
example(int a, int b)
{

	if (a == b)
		return "comparison, not parenthesized";

	/*
	 * Clang-Tidy marks a comparison with extra parentheses as an error,
	 * expecting that assignments are parenthesized and comparisons
	 * aren't.
	 */
	if ((a == b))
		return "comparison, parenthesized";

	if (
# 25 "msg_159.c" 3 4
	    (a == b)
# 27 "msg_159.c"
	    )
		return "comparison, parenthesized, from system header";

	/* expect+1: warning: assignment in conditional context [159] */
	if (a = b)
		return "assignment, not parenthesized";

	/*
	 * GCC established the convention that an assignment that is
	 * parenthesized is intended as an assignment, so don't warn about
	 * that case.
	 */
	if ((a = b))
		return "assignment, parenthesized";

	if ((a = b) != 0)
		return "explicit comparison after assignment";

	return "other";
}
