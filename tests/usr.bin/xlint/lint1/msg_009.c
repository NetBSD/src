/*	$NetBSD: msg_009.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_009.c"

// Test for message: only register valid as formal parameter storage class [9]

extern void typedef_example(typedef int param);	/* expect: 9 */
extern void auto_example(auto int param);	/* expect: 9 */
extern void static_example(static int param);	/* expect: 9 */
extern void extern_example(extern int param);	/* expect: 9 */
