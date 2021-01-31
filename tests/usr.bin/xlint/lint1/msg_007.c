/*	$NetBSD: msg_007.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_007.c"

// Test for message: only one storage class allowed [7]

extern static void example(void);	/* expect: 7 */
extern extern_function(void);
