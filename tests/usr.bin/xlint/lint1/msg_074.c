/*	$NetBSD: msg_074.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_074.c"

// Test for message: no hex digits follow \x [74]

char invalid_hex = '\x';		/* expect: 74 */
char invalid_hex_letter = '\xg';	/* expect: 74, 294 */
char valid_hex = '\xff';
char valid_single_digit_hex = '\xa';
