/*	$NetBSD: msg_188.c,v 1.8 2025/04/12 15:57:26 rillig Exp $	*/
# 3 "msg_188.c"

/* Test for message: automatic aggregate initialization requires C90 or later [188] */

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
	/* expect+2: warning: automatic aggregate initialization requires C90 or later [188] */
	/* expect+1: warning: 'local' set but not used in function 'function' [191] */
	struct point local = {
		3,
		4,
	};
}
