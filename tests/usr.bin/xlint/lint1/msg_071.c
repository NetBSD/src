/*	$NetBSD: msg_071.c,v 1.6 2023/01/22 16:05:08 rillig Exp $	*/
# 3 "msg_071.c"

// Test for message: too many characters in character constant [71]

/*
 * See also:
 *	lex_char.c
 *	lex_char_uchar.c
 *	lex_wide_char.c
 */

/*
 * C11 6.4.4.4p7 says: Each hexadecimal escape sequence is the longest
 * sequence of characters that can constitute the escape sequence.
 */
char valid_multi_digit_hex = '\x0000000000000000000000a';
/* expect+2: error: too many characters in character constant [71] */
/* expect+1: warning: initializer does not fit [178] */
char invalid_multi_digit_hex = '\x000g000000000000000000a';
