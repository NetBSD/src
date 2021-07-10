/*	$NetBSD: d_c99_anon_struct.c,v 1.4 2021/07/10 10:56:31 rillig Exp $	*/
# 3 "d_c99_anon_struct.c"

/* Anonymous struct test */

typedef int type;

struct point {
	int x;
	int y;
};

struct rect {
	struct {
		struct point top_left;
		struct point bottom_right;
	};
	type z;
};


int
main(void)
{
	struct rect r;
	r.top_left.x = 1;
	return 0;
}
