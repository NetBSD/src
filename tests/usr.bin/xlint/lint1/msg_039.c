/*	$NetBSD: msg_039.c,v 1.3 2022/04/05 23:09:19 rillig Exp $	*/
# 3 "msg_039.c"

/* Test for message: zero sized array in struct is a C99 extension: %s [39] */

/* lint1-flags: -w */

struct s {
	/* expect+2: warning: zero sized array is a C99 extension [322] */
	/* expect+1: warning: zero sized array in struct is a C99 extension: member [39] */
	char member[0];
	char member2;
};
