/*	$NetBSD: msg_075.c,v 1.9 2024/02/03 18:58:05 rillig Exp $	*/
# 3 "msg_075.c"

// Test for message: overflow in hex escape [75]

/* lint1-extra-flags: -X 351 */

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

char char_hex1 = '\xf';
char char_hex2 = '\xff';
/* expect+1: warning: overflow in hex escape [75] */
char char_hex3 = '\x100';
/* expect+1: warning: overflow in hex escape [75] */
char char_hex4 = '\xffff';
/* expect+1: warning: overflow in hex escape [75] */
char char_hex5 = '\xfffff';
/* expect+1: warning: overflow in hex escape [75] */
char char_hex9 = '\xfffffffff';

int wide_hex1 = L'\xf';
int wide_hex2 = L'\xff';
int wide_hex3 = L'\x100';
int wide_hex4 = L'\xffff';
int wide_hex5 = L'\xfffff';
/* expect+1: warning: overflow in hex escape [75] */
int wide_hex9 = L'\xfffffffff';

char char_string_hex1[] = "\xf";
char char_string_hex2[] = "\xff";
/* expect+1: warning: overflow in hex escape [75] */
char char_string_hex3[] = "\x100";
/* expect+1: warning: overflow in hex escape [75] */
char char_string_hex4[] = "\xffff";
/* expect+1: warning: overflow in hex escape [75] */
char char_string_hex5[] = "\xfffff";
/* expect+1: warning: overflow in hex escape [75] */
char char_string_hex9[] = "\xfffffffff";

int wide_string_hex1[] = L"\xf";
int wide_string_hex2[] = L"\xff";
int wide_string_hex3[] = L"\x100";
int wide_string_hex4[] = L"\xffff";
int wide_string_hex5[] = L"\xfffff";
/* expect+1: warning: overflow in hex escape [75] */
int wide_string_hex9[] = L"\xfffffffff";
