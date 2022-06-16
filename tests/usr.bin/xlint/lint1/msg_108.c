/*	$NetBSD: msg_108.c,v 1.6 2022/06/16 16:58:36 rillig Exp $	*/
# 3 "msg_108.c"

// Test for message: operand of '%s' has invalid type (%s) [108]

/*
 * Before tree.c 1.137 from 2021-01-19, taking the complement of a struct
 * (an absurd idea, by the way), resulted in an internal error because the
 * message 108 had two operands, the second of which was always NOTSPEC, as
 * could be expected for a unary operator.
 *
 * Since an error "invalid type (none)" doesn't make sense, lint rather
 * chooses to crash than to generate such an error.
 */
void
complement_of_a_struct(void)
{
	struct s {
		int member;
	} s = {
	    0
	};

	/* expect+1: error: operand of '~' has invalid type (struct) [108] */
	s = ~s;
}
