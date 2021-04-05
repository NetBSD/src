/*	$NetBSD: msg_074.c,v 1.4 2021/04/05 01:35:34 rillig Exp $	*/
# 3 "msg_074.c"

// Test for message: no hex digits follow \x [74]

char invalid_hex = '\x';		/* expect: 74 */
char invalid_hex_letter = '\xg';	/* expect: 74 *//* expect: 294 */
char valid_hex = '\xff';
char valid_single_digit_hex = '\xa';
