/*	$NetBSD: msg_319.c,v 1.5 2023/03/28 14:44:35 rillig Exp $	*/
# 3 "msg_319.c"

/* Test for message: compound literals are a C99/GCC extension [319] */

/* lint1-flags: -sw -X 351 */

/* expect+2: error: compound literals are a C99/GCC extension [319] */
/* expect+1: error: non-constant initializer [177] */
int number = (int) { 3 };

struct point {
	int x;
	int y;
} point = (struct point) {
	3,
	4,
};
/* expect-1: error: compound literals are a C99/GCC extension [319] */
/* expect-2: error: {}-enclosed initializer required [181] */
