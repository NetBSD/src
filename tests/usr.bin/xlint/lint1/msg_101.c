/*	$NetBSD: msg_101.c,v 1.2 2021/01/08 21:25:03 rillig Exp $	*/
# 3 "msg_101.c"

// Test for message: undefined struct/union member: %s [101]

struct point {
	int x, y;
};

int
get_z(const struct point *p)
{
	return p.z;
}
