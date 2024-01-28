/*	$NetBSD: msg_188.c,v 1.7 2024/01/28 08:17:27 rillig Exp $	*/
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
	/* expect+2: warning: no automatic aggregate initialization in traditional C [188] */
	/* expect+1: warning: 'local' set but not used in function 'function' [191] */
	struct point local = {
		3,
		4,
	};
}
