/*	$NetBSD: msg_254.c,v 1.5 2024/02/02 16:25:58 rillig Exp $	*/
# 3 "msg_254.c"

/* Test for message: newline in string or char constant [254] */

/* lint1-flags: -tw -q17 */

/*
 * The sequence backslash-newline is a GCC extension.
 * C99 does not allow it.
 */

/* expect+6: error: newline in string or char constant [254] */
/* expect+5: error: unterminated string constant [258] */
/* expect+4: error: syntax error '"' [249] */
/* expect+4: error: newline in string or char constant [254] */
/* expect+3: error: unterminated string constant [258] */
"line1
line2"
