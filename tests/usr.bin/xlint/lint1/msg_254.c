/*	$NetBSD: msg_254.c,v 1.6 2024/02/02 19:07:58 rillig Exp $	*/
# 3 "msg_254.c"

/* Test for message: newline in string or char constant [254] */

/* lint1-flags: -tw -q17 */

/*
 * A literal newline must not occur in a character constant or string literal.
 */

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
