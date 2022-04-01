/*	$NetBSD: msg_014.c,v 1.4 2022/04/01 23:16:32 rillig Exp $	*/
# 3 "msg_014.c"

// Test for message: compiler takes alignment of function [14]

typedef void function(void);

/* expect+1: error: cannot take size/alignment of function type 'function(void) returning void' [144] */
unsigned long alignof_function = __alignof__(function);

TODO: "Add example code that triggers the above message." /* expect: 249 */
TODO: "Add example code that almost triggers the above message."
