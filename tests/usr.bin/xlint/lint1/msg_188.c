/*	$NetBSD: msg_188.c,v 1.3 2021/03/28 15:12:20 rillig Exp $	*/
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
	struct point local = {	/* expect: 188 */
		3,
		4,
	};
}
