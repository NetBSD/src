/*	$NetBSD: msg_353.c,v 1.4 2023/07/14 08:53:52 rillig Exp $	*/
# 3 "msg_353.c"

// Test for message 353: empty initializer braces require C23 or later [353]
//
// See also:
//	c23.c

/* lint1-extra-flags: -Ac11 -X 351 */

void
empty_initializer_braces(void)
{
	struct s {
		int member;
	} s;

	/* expect+1: error: empty initializer braces require C23 or later [353] */
	s = (struct s){};
	s = (struct s){0};
}
