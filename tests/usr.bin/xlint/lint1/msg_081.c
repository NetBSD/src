/*	$NetBSD: msg_081.c,v 1.7.2.1 2025/08/02 05:58:15 perseant Exp $	*/
# 3 "msg_081.c"

/* Test for message: \a requires C90 or later [81] */

/* lint1-flags: -tw -X 351 */

/* expect+1: warning: \a requires C90 or later [81] */
char char_a = '\a';
/* expect+1: warning: \a requires C90 or later [81] */
char char_string_a[] = "The bell\a rings";
