/*	$NetBSD: msg_007.c,v 1.5 2022/10/01 09:42:40 rillig Exp $	*/
# 3 "msg_007.c"

// Test for message: only one storage class allowed [7]

/* expect+1: error: only one storage class allowed [7] */
extern static void example(void);

/* expect+1: error: old-style declaration; add 'int' [1] */
extern extern_function(void);
