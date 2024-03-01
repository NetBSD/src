/*	$NetBSD: msg_349.c,v 1.3 2024/03/01 17:22:55 rillig Exp $	*/
# 3 "msg_349.c"

// Test for message: non type argument to alignof is a GCC extension [349]

/* lint1-flags: -S -w -X 351 */

unsigned long c11_type = _Alignof(int);

/* expect+1: warning: non type argument to alignof is a GCC extension [349] */
unsigned long c11_expr = _Alignof(3);

unsigned long gcc_type = __alignof__(int);

/* expect+1: warning: non type argument to alignof is a GCC extension [349] */
unsigned long gcc_expr = __alignof__(3);
