/*	$NetBSD: msg_009.c,v 1.6 2023/07/09 11:01:27 rillig Exp $	*/
# 3 "msg_009.c"

// Test for message: only 'register' is valid as storage class in parameter [9]

/* lint1-extra-flags: -X 351 */

/* expect+1: error: only 'register' is valid as storage class in parameter [9] */
extern void typedef_example(typedef int param);
/* expect+1: error: only 'register' is valid as storage class in parameter [9] */
extern void auto_example(auto int param);
/* expect+1: error: only 'register' is valid as storage class in parameter [9] */
extern void static_example(static int param);
/* expect+1: error: only 'register' is valid as storage class in parameter [9] */
extern void extern_example(extern int param);
