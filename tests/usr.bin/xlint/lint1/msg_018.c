/*	$NetBSD: msg_018.c,v 1.3 2021/08/26 19:23:25 rillig Exp $	*/
# 3 "msg_018.c"

// Test for message: illegal use of 'void' [18]

/* expect+1: error: void type for 'x' [19] */
void x;

/* expect+1: error: cannot take size/alignment of void [146] */
unsigned long sizeof_void = sizeof(void);

TODO: "Add example code that triggers the above message." /* expect: 249 */
TODO: "Add example code that almost triggers the above message."
