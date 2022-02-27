/*	$NetBSD: msg_049.c,v 1.4 2022/02/27 11:40:30 rillig Exp $	*/
# 3 "msg_049.c"

/* Test for message: anonymous struct/union members is a C11 feature [49] */

/* lint1-flags: -sw */

/*
 * FIXME: C99 does not allow anonymous struct/union members, that's a GCC
 * extension that got incorporated into C11.
 */

struct {
	unsigned int flag: 1;

	/*
	 * This is an anonymous struct/union member, but that's not what
	 * message 49 is about.
	 */
	unsigned int :0;

	union {
		int int_value;
		void *pointer_value;
	};
	/* expect-1: warning: anonymous struct/union members is a C11 feature [49] */
} s;
