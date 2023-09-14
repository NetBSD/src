/*	$NetBSD: msg_039.c,v 1.5 2023/09/14 21:53:02 rillig Exp $	*/
# 3 "msg_039.c"

/* Test for message: zero-sized array '%s' in struct requires C99 or later [39] */

/* lint1-flags: -w */

struct s {
	/* expect+2: warning: zero sized array requires C99 or later [322] */
	/* expect+1: warning: zero-sized array 'member' in struct requires C99 or later [39] */
	char member[0];
	char member2;
};
