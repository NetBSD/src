/*	$NetBSD: d_c99_anon_struct.c,v 1.3 2021/01/31 14:39:31 rillig Exp $	*/
# 3 "d_c99_anon_struct.c"

/* Anonymous struct test */

typedef int type;

struct point {
	int x;
	int y;
};

struct bar {
	struct {
		struct point top_left;
		struct point bottom_right;
	};
	type z;
};


int
main(void)
{
	struct bar b;
	b.top_left.x = 1;
	return 0;
}
