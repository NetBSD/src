/*	$NetBSD: msg_009.c,v 1.5 2023/03/28 14:44:34 rillig Exp $	*/
# 3 "msg_009.c"

// Test for message: only register valid as formal parameter storage class [9]

/* lint1-extra-flags: -X 351 */

/* expect+1: error: only register valid as formal parameter storage class [9] */
extern void typedef_example(typedef int param);
/* expect+1: error: only register valid as formal parameter storage class [9] */
extern void auto_example(auto int param);
/* expect+1: error: only register valid as formal parameter storage class [9] */
extern void static_example(static int param);
/* expect+1: error: only register valid as formal parameter storage class [9] */
extern void extern_example(extern int param);
