/*	$NetBSD: msg_069.c,v 1.4 2022/06/17 18:54:53 rillig Exp $	*/
# 3 "msg_069.c"

// Test for message: inappropriate qualifiers with 'void' [69]

/* expect+2: error: void type for 'const_void' [19] */
/* expect+1: warning: inappropriate qualifiers with 'void' [69] */
const void const_void;

/* expect+2: error: void type for 'volatile_void' [19] */
/* expect+1: warning: inappropriate qualifiers with 'void' [69] */
volatile void volatile_void;
