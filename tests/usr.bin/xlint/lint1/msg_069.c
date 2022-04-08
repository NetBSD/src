/*	$NetBSD: msg_069.c,v 1.3 2022/04/08 21:29:29 rillig Exp $	*/
# 3 "msg_069.c"

// Test for message: inappropriate qualifiers with 'void' [69]

/* expect+2: error: void type for 'const_void' [19] */
/* expect+1: warning: inappropriate qualifiers with 'void' */
const void const_void;

/* expect+2: error: void type for 'volatile_void' [19] */
/* expect+1: warning: inappropriate qualifiers with 'void' */
volatile void volatile_void;
