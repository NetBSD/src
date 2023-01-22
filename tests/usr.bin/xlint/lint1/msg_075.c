/*	$NetBSD: msg_075.c,v 1.5 2023/01/22 16:05:08 rillig Exp $	*/
# 3 "msg_075.c"

// Test for message: overflow in hex escape [75]

/*
 * See also:
 *	lex_char.c
 *	lex_char_uchar.c
 *	lex_string.c
 *	lex_wide_char.c
 *	lex_wide_string.c
 */

/* expect+1: warning: overflow in hex escape [75] */
char str[] = "\x12345678123456781234567812345678";

/* C11 6.4.4.4p7 */
char leading_zeroes = '\x0000000000000000000000000000020';
