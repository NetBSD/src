/*	$NetBSD: msg_081.c,v 1.7 2024/02/02 19:07:58 rillig Exp $	*/
# 3 "msg_081.c"

/* Test for message: \a undefined in traditional C [81] */

/* lint1-flags: -tw -X 351 */

/* expect+1: warning: \a undefined in traditional C [81] */
char char_a = '\a';
/* expect+1: warning: \a undefined in traditional C [81] */
char char_string_a[] = "The bell\a rings";
