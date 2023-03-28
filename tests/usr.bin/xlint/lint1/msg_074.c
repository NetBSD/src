/*	$NetBSD: msg_074.c,v 1.6 2023/03/28 14:44:34 rillig Exp $	*/
# 3 "msg_074.c"

// Test for message: no hex digits follow \x [74]

/* lint1-extra-flags: -X 351 */

/* expect+1: error: no hex digits follow \x [74] */
char invalid_hex = '\x';

/* expect+2: error: no hex digits follow \x [74] */
/* expect+1: warning: multi-character character constant [294] */
char invalid_hex_letter = '\xg';

char valid_hex = '\xff';
char valid_single_digit_hex = '\xa';
