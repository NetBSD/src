/*	$NetBSD: msg_074.c,v 1.7 2024/02/02 19:07:58 rillig Exp $	*/
# 3 "msg_074.c"

// Test for message: no hex digits follow \x [74]

/* lint1-extra-flags: -X 351 */

/* expect+1: error: no hex digits follow \x [74] */
char char_invalid_hex = '\x';
/* expect+2: error: no hex digits follow \x [74] */
/* expect+1: warning: multi-character character constant [294] */
char char_invalid_hex_letter = '\xg';
char char_hex1 = '\xf';
char char_hex2 = '\xff';

/* expect+1: error: no hex digits follow \x [74] */
int wide_invalid_hex = L'\x';
/* expect+2: error: no hex digits follow \x [74] */
/* expect+1: error: too many characters in character constant [71] */
int wide_invalid_hex_letter = L'\xg';
int wide_hex1 = L'\xf';
int wide_hex2 = L'\xff';

/* expect+1: error: no hex digits follow \x [74] */
char char_string_invalid_hex[] = "\x";
/* expect+1: error: no hex digits follow \x [74] */
char char_string_invalid_hex_letter[] = "\xg";
char char_string_hex1[] = "\xf";
char char_string_hex2[] = "\xff";

/* expect+1: error: no hex digits follow \x [74] */
int wide_string_invalid_hex[] = L"\x";
/* expect+1: error: no hex digits follow \x [74] */
int wide_string_invalid_hex_letter[] = L"\xg";
int wide_string_hex1[] = L"\xf";
int wide_string_hex2[] = L"\xff";
