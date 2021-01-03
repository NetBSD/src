/*	$NetBSD: msg_074.c,v 1.2 2021/01/03 15:35:00 rillig Exp $	*/
# 3 "msg_074.c"

// Test for message: no hex digits follow \x [74]

char invalid_hex = '\x';
char invalid_hex_letter = '\xg';
char valid_hex = '\xff';
char valid_single_digit_hex = '\xa';
