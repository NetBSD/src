/*	$NetBSD: msg_039.c,v 1.4 2022/06/20 21:13:36 rillig Exp $	*/
# 3 "msg_039.c"

/* Test for message: zero-sized array '%s' in struct is a C99 extension [39] */

/* lint1-flags: -w */

struct s {
	/* expect+2: warning: zero sized array is a C99 extension [322] */
	/* expect+1: warning: zero-sized array 'member' in struct is a C99 extension [39] */
	char member[0];
	char member2;
};
