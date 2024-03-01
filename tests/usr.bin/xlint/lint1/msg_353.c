/*	$NetBSD: msg_353.c,v 1.5 2024/03/01 17:22:55 rillig Exp $	*/
# 3 "msg_353.c"

// Test for message: empty initializer braces require C23 or later [353]
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
