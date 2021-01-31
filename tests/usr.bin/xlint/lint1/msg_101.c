/*	$NetBSD: msg_101.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_101.c"

// Test for message: undefined struct/union member: %s [101]

struct point {
	int x, y;
};

int
get_z(const struct point *p)
{
	return p.z;		/* expect: 101 */
}
