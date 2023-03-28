/*	$NetBSD: msg_081.c,v 1.6 2023/03/28 14:44:34 rillig Exp $	*/
# 3 "msg_081.c"

/* Test for message: \a undefined in traditional C [81] */

/* lint1-flags: -tw -X 351 */

/* expect+1: warning: \a undefined in traditional C [81] */
char str[] = "The bell\a rings";
