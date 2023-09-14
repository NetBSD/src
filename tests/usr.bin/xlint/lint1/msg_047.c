/*	$NetBSD: msg_047.c,v 1.6 2023/09/14 21:53:02 rillig Exp $	*/
# 3 "msg_047.c"

/* Test for message: zero sized %s is a C99 feature [47] */

/* lint1-flags: -sw */

struct empty {
	/* TODO: The C99 syntax in 6.7.2.1 requires at least 1 member. */
};
/* expect-1: error: zero sized struct is a C99 feature [47] */

struct zero_sized {
	/* expect+2: error: zero sized array requires C99 or later [322] */
	/* expect+1: error: zero-sized array 'dummy' in struct requires C99 or later [39] */
	char dummy[0];
};
/* expect-1: error: zero sized struct is a C99 feature [47] */
