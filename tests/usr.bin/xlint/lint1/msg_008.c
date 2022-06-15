/*	$NetBSD: msg_008.c,v 1.4 2022/06/15 20:18:31 rillig Exp $	*/
# 3 "msg_008.c"

// Test for message: illegal storage class [8]

typedef void
example(void)
/* expect+1: error: illegal storage class [8] */
{
}
