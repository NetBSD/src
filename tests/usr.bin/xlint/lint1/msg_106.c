/*	$NetBSD: msg_106.c,v 1.4 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_106.c"

// Test for message: left operand of '->' must be pointer [106]
// This message is not used.

/* lint1-extra-flags: -X 351 */

struct s {
	int member;
};

void example(struct s s)
{
	s.member++;

	/* expect+1: error: left operand of '->' must be pointer to struct or union, not 'struct s' [104] */
	return s->member;
}
