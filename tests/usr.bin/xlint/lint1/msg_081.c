/*	$NetBSD: msg_081.c,v 1.4 2022/04/16 13:25:27 rillig Exp $	*/
# 3 "msg_081.c"

/* Test for message: \a undefined in traditional C [81] */

/* lint1-flags: -tw */

char str[] = "The bell\a rings";	/* expect: 81 */
