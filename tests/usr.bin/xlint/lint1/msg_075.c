/*	$NetBSD: msg_075.c,v 1.10 2024/02/03 19:18:36 rillig Exp $	*/
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

char char_hex4bit = '\xf';
char char_hex7bit = '\x7f';
char char_hex8bit = '\xff';
/* expect+1: warning: overflow in hex escape [75] */
char char_hex9bit = '\x100';
/* expect+1: warning: overflow in hex escape [75] */
char char_hex16bit = '\xffff';
/* expect+1: warning: overflow in hex escape [75] */
char char_hex20bit = '\xfffff';
/* expect+1: warning: overflow in hex escape [75] */
char char_hex31bit = '\x7fffffff';
/* expect+1: warning: overflow in hex escape [75] */
char char_hex32bit = '\xffffffff';
/* expect+1: warning: overflow in hex escape [75] */
char char_hex33bit = '\x1000000ff';
/* expect+1: warning: overflow in hex escape [75] */
char char_hex36bit = '\xfffffffff';

int wide_hex4bit = L'\xf';
int wide_hex7bit = L'\x7f';
int wide_hex8bit = L'\xff';
int wide_hex9bit = L'\x100';
int wide_hex16bit = L'\xffff';
int wide_hex20bit = L'\xfffff';
int wide_hex31bit = L'\x7fffffff';
int wide_hex32bit = L'\xffffffff';
/* expect+1: warning: overflow in hex escape [75] */
int wide_hex33bit = L'\x1000000ff';
/* expect+1: warning: overflow in hex escape [75] */
int wide_hex36bit = L'\xfffffffff';

char char_string_hex4bit[] = "\xf";
char char_string_hex7bit[] = "\x7f";
char char_string_hex8bit[] = "\xff";
/* expect+1: warning: overflow in hex escape [75] */
char char_string_hex9bit[] = "\x100";
/* expect+1: warning: overflow in hex escape [75] */
char char_string_hex16bit[] = "\xffff";
/* expect+1: warning: overflow in hex escape [75] */
char char_string_hex20bit[] = "\xfffff";
/* expect+1: warning: overflow in hex escape [75] */
char char_string_hex31bit[] = "\x7fffffff";
/* expect+1: warning: overflow in hex escape [75] */
char char_string_hex32bit[] = "\xffffffff";
/* expect+1: warning: overflow in hex escape [75] */
char char_string_hex33bit[] = "\x1000000ff";
/* expect+1: warning: overflow in hex escape [75] */
char char_string_hex36[] = "\xfffffffff";

int wide_string_hex4bit[] = L"\xf";
int wide_string_hex7bit[] = L"\x7f";
int wide_string_hex8bit[] = L"\xff";
int wide_string_hex9bit[] = L"\x100";
int wide_string_hex16bit[] = L"\xffff";
int wide_string_hex20bit[] = L"\xfffff";
int wide_string_hex31bit[] = L"\x7fffffff";
int wide_string_hex32bit[] = L"\xffffffff";
/* expect+1: warning: overflow in hex escape [75] */
int wide_string_hex33bit[] = L"\x1000000ff";
/* expect+1: warning: overflow in hex escape [75] */
int wide_string_hex36bit[] = L"\xfffffffff";
