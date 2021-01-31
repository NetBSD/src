/*	$NetBSD: msg_008.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_008.c"

// Test for message: illegal storage class [8]

typedef void
example(void)
{				/* expect: 8 */
}
