/*	$NetBSD: msg_009.c,v 1.2 2021/01/07 18:37:41 rillig Exp $	*/
# 3 "msg_009.c"

// Test for message: only register valid as formal parameter storage class [9]

extern void typedef_example(typedef int param);
extern void auto_example(auto int param);
extern void static_example(static int param);
extern void extern_example(extern int param);
