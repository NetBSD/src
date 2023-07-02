/*	$NetBSD: c23.c,v 1.1 2023/07/02 23:45:10 rillig Exp $	*/
# 3 "c23.c"

// Tests for the option -Ac23, which allows features from C23 and all earlier
// ISO standards, but none of the GNU extensions.
//
// See also:
//	msg_353.c

/* lint1-flags: -Ac23 -w -X 351 */

int
c23(void)
{
	struct s {
		int member;
	} s;

	s = (struct s){};
	s = (struct s){s.member};
	return s.member;
}
