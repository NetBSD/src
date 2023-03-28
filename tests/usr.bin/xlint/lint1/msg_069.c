/*	$NetBSD: msg_069.c,v 1.5 2023/03/28 14:44:34 rillig Exp $	*/
# 3 "msg_069.c"

// Test for message: inappropriate qualifiers with 'void' [69]

/* lint1-extra-flags: -X 351 */

/* expect+2: error: void type for 'const_void' [19] */
/* expect+1: warning: inappropriate qualifiers with 'void' [69] */
const void const_void;

/* expect+2: error: void type for 'volatile_void' [19] */
/* expect+1: warning: inappropriate qualifiers with 'void' [69] */
volatile void volatile_void;
