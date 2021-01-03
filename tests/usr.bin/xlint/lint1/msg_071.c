/*	$NetBSD: msg_071.c,v 1.2 2021/01/03 15:35:00 rillig Exp $	*/
# 3 "msg_071.c"

// Test for message: too many characters in character constant [71]

/*
 * C11 6.4.4.4p7 says: Each hexadecimal escape sequence is the longest
 * sequence of characters that can constitute the escape sequence.
 */
char valid_multi_digit_hex = '\x0000000000000000000000a';
char invalid_multi_digit_hex = '\x000g000000000000000000a';
