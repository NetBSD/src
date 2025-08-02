/*	$NetBSD: msg_082.c,v 1.7.2.1 2025/08/02 05:58:15 perseant Exp $	*/
# 3 "msg_082.c"

/* Test for message: \x requires C90 or later [82] */

/* lint1-flags: -Stw -X 351 */

/* expect+1: warning: \x requires C90 or later [82] */
char char_hex = '\x78';
/* expect+1: warning: \x requires C90 or later [82] */
char char_string_hex[] = "\x78";
