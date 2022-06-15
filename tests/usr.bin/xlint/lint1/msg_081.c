/*	$NetBSD: msg_081.c,v 1.5 2022/06/15 20:18:31 rillig Exp $	*/
# 3 "msg_081.c"

/* Test for message: \a undefined in traditional C [81] */

/* lint1-flags: -tw */

/* expect+1: warning: \a undefined in traditional C [81] */
char str[] = "The bell\a rings";
