/*	$NetBSD: msg_106.c,v 1.3 2022/06/15 20:18:31 rillig Exp $	*/
# 3 "msg_106.c"

// Test for message: left operand of '->' must be pointer [106]
// This message is not used.

struct s {
	int member;
};

void example(struct s s)
{
	s.member++;

	/* expect+1: error: left operand of '->' must be pointer to struct or union, not 'struct s' [104] */
	return s->member;
}
