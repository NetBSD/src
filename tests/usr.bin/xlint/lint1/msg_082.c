/*	$NetBSD: msg_082.c,v 1.8 2025/04/12 15:57:26 rillig Exp $	*/
# 3 "msg_082.c"

/* Test for message: \x requires C90 or later [82] */

/* lint1-flags: -Stw -X 351 */

/* expect+1: warning: \x requires C90 or later [82] */
char char_hex = '\x78';
/* expect+1: warning: \x requires C90 or later [82] */
char char_string_hex[] = "\x78";
