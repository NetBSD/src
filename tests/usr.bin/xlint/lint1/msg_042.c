/*	$NetBSD: msg_042.c,v 1.3 2022/04/08 21:29:29 rillig Exp $	*/
# 3 "msg_042.c"

/* Test for message: forward reference to enum type [42] */

/* lint1-extra-flags: -p */

/* expect+1: warning: forward reference to enum type [42] */
enum forward;

enum forward {
	defined
};
