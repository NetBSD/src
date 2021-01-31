/*	$NetBSD: msg_081.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_081.c"

// Test for message: \a undefined in traditional C [81]

/* lint1-flags: -Stw */

char str[] = "The bell\a rings";	/* expect: 81 */
