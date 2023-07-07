/*	$NetBSD: msg_104.c,v 1.5 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_104.c"

// Test for message: left operand of '->' must be pointer to struct or union, not '%s' [104]

/* lint1-extra-flags: -X 351 */

struct point {
	int x, y;
};

/* ARGSUSED */
void
test(struct point pt, struct point *ptr)
{
	/* expect+1: error: left operand of '->' must be pointer to struct or union, not 'struct point' [104] */
	pt->x = 0;
	ptr->y = 0;
}
