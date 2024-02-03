/*	$NetBSD: msg_074.c,v 1.8 2024/02/03 19:18:36 rillig Exp $	*/
# 3 "msg_074.c"

// Test for message: no hex digits follow \x [74]
//
// See also:
//	msg_075.c		overflow in hex escape

/* lint1-extra-flags: -X 351 */

/* expect+1: error: no hex digits follow \x [74] */
char char_invalid_hex = '\x';
/* expect+2: error: no hex digits follow \x [74] */
/* expect+1: warning: multi-character character constant [294] */
char char_invalid_hex_letter = '\xg';

/* expect+1: error: no hex digits follow \x [74] */
int wide_invalid_hex = L'\x';
/* expect+2: error: no hex digits follow \x [74] */
/* expect+1: error: too many characters in character constant [71] */
int wide_invalid_hex_letter = L'\xg';

/* expect+1: error: no hex digits follow \x [74] */
char char_string_invalid_hex[] = "\x";
/* expect+1: error: no hex digits follow \x [74] */
char char_string_invalid_hex_letter[] = "\xg";

/* expect+1: error: no hex digits follow \x [74] */
int wide_string_invalid_hex[] = L"\x";
/* expect+1: error: no hex digits follow \x [74] */
int wide_string_invalid_hex_letter[] = L"\xg";
