/*	$NetBSD: msg_104.c,v 1.3 2021/07/04 17:01:58 rillig Exp $	*/
# 3 "msg_104.c"

// Test for message: left operand of '->' must be pointer to struct or union, not '%s' [104]

struct point {
	int x, y;
};

/* ARGSUSED */
void
test(struct point pt, struct point *ptr)
{
	/* expect+1: left operand of '->' must be pointer to struct or union, not 'struct point' [104] */
	pt->x = 0;
	ptr->y = 0;
}
