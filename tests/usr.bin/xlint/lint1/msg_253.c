/*	$NetBSD: msg_253.c,v 1.5 2024/02/02 16:05:37 rillig Exp $	*/
# 3 "msg_253.c"

// Test for message: unterminated character constant [253]

/* expect+4: error: newline in string or char constant [254] */
/* expect+3: error: unterminated character constant [253] */
/* expect+2: error: syntax error ''' [249] */
'
