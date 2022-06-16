/*	$NetBSD: msg_258.c,v 1.3 2022/06/16 21:24:41 rillig Exp $	*/
# 3 "msg_258.c"

// Test for message: unterminated string constant [258]

int dummy;

// A string literal that is not finished at the end of the line confuses the
// parser.
//
// "This string doesn't end in this line.

/* expect+4: error: unterminated string constant [258] */
/* expect+3: error: syntax error '' [249] */
/* expect+2: error: empty array declaration: str [190] */
const char str[] = "This is the end.
