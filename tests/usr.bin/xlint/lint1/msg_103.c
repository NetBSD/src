/*	$NetBSD: msg_103.c,v 1.4 2022/06/17 18:54:53 rillig Exp $	*/
# 3 "msg_103.c"

// Test for message: left operand of '.' must be struct or union, not '%s' [103]

struct point {
	int x, y;
};

void
test(struct point pt, struct point *ptr)
{
	pt.x = 0;
	/* expect+1: error: left operand of '.' must be struct or union, not 'pointer to struct point' [103] */
	ptr.y = 0;
}
