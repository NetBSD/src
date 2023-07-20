/*	$NetBSD: msg_103.c,v 1.5 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_103.c"

// Test for message: left operand of '.' must be struct or union, not '%s' [103]

/* lint1-extra-flags: -X 351 */

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
