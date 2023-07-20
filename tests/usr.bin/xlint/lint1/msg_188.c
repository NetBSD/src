/*	$NetBSD: msg_188.c,v 1.6 2023/07/07 06:03:31 rillig Exp $	*/
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
	/* expect+2: warning: 'local' set but not used in function 'function' [191] */
	/* expect+1: warning: no automatic aggregate initialization in traditional C [188] */
	struct point local = {
		3,
		4,
	};
}
