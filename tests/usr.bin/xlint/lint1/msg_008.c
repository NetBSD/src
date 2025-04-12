/*	$NetBSD: msg_008.c,v 1.5 2025/04/12 15:49:50 rillig Exp $	*/
# 3 "msg_008.c"

// Test for message: invalid storage class [8]

typedef void
example(void)
/* expect+1: error: invalid storage class [8] */
{
}
