/*	$NetBSD: msg_082.c,v 1.7 2024/02/02 19:07:58 rillig Exp $	*/
# 3 "msg_082.c"

/* Test for message: \x undefined in traditional C [82] */

/* lint1-flags: -Stw -X 351 */

/* expect+1: warning: \x undefined in traditional C [82] */
char char_hex = '\x78';
/* expect+1: warning: \x undefined in traditional C [82] */
char char_string_hex[] = "\x78";
