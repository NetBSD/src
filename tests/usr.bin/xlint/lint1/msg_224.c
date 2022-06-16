/*	$NetBSD: msg_224.c,v 1.4 2022/06/16 21:24:41 rillig Exp $	*/
# 3 "msg_224.c"

// Test for message: cannot recover from previous errors [224]

/* expect+1: error: syntax error 'error' [249] */
void example1(void) { "syntax" error; }
/* expect+1: error: syntax error 'error' [249] */
void example2(void) { "syntax" error; }
/* expect+1: error: syntax error 'error' [249] */
void example3(void) { "syntax" error; }
/* expect+1: error: syntax error 'error' [249] */
void example4(void) { "syntax" error; }
/* expect+2: error: syntax error 'error' [249] */
/* expect+1: error: cannot recover from previous errors [224] */
void example5(void) { "syntax" error; }
void example6(void) { "syntax" error; }
