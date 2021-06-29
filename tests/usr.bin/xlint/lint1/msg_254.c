/*	$NetBSD: msg_254.c,v 1.3 2021/06/29 07:17:43 rillig Exp $	*/
# 3 "msg_254.c"

/* Test for message: newline in string or char constant [254] */

/* lint1-flags: -tw */

/*
 * The sequence backslash-newline is a GCC extension.
 * C99 does not allow it.
 */

/* expect+6: newline in string or char constant [254] */
/* expect+5: unterminated string constant [258] */
/* expect+4: syntax error '"' [249] */
/* expect+4: newline in string or char constant [254] */
/* expect+3: unterminated string constant [258] */
"line1
line2"
