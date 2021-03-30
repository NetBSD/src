/*	$NetBSD: msg_101.c,v 1.4 2021/03/30 15:18:19 rillig Exp $	*/
# 3 "msg_101.c"

// Test for message: type '%s' does not have member '%s' [101]

struct point {
	int x, y;
};

int
get_z(const struct point *p)
{
	return p.z;		/* expect: 101 */
}
