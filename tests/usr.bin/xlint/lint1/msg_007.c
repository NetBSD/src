/*	$NetBSD: msg_007.c,v 1.6 2023/03/28 14:44:34 rillig Exp $	*/
# 3 "msg_007.c"

// Test for message: only one storage class allowed [7]

/* lint1-extra-flags: -X 351 */

/* expect+1: error: only one storage class allowed [7] */
extern static void example(void);

/* expect+1: error: old-style declaration; add 'int' [1] */
extern extern_function(void);
