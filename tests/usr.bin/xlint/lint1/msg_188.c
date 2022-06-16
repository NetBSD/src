/*	$NetBSD: msg_188.c,v 1.4 2022/06/16 16:58:36 rillig Exp $	*/
# 3 "msg_188.c"

/* Test for message: no automatic aggregate initialization in traditional C [188] */

/* lint1-flags: -tw */

struct point {
	int x;
	int y;
};

struct point global = {
	3,
	4,
};

void
function()
{
	/* expect+1: warning: no automatic aggregate initialization in traditional C [188] */
	struct point local = {
		3,
		4,
	};
}
