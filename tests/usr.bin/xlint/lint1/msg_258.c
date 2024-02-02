/*	$NetBSD: msg_258.c,v 1.7 2024/02/02 19:07:58 rillig Exp $	*/
# 3 "msg_258.c"

// Test for message: unterminated string constant [258]

/* lint1-extra-flags: -X 351 */

/* expect+3: error: newline in string or char constant [254] */
/* expect+2: error: unterminated character constant [253] */
char char_incomplete = 'x
;
/* expect+3: error: newline in string or char constant [254] */
/* expect+2: error: unterminated string constant [258] */
char char_string_incomplete[] = "x
;
/* expect+3: error: newline in string or char constant [254] */
/* expect+2: error: unterminated character constant [253] */
int wide_incomplete = L'x
;
/* expect+3: error: newline in string or char constant [254] */
/* expect+2: error: unterminated string constant [258] */
int wide_string_incomplete[] = L"x
;
