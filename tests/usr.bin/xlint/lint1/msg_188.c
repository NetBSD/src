/*	$NetBSD: msg_188.c,v 1.5 2023/03/28 14:44:35 rillig Exp $	*/
# 3 "msg_188.c"

/* Test for message: no automatic aggregate initialization in traditional C [188] */

/* lint1-flags: -tw -X 351 */

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
