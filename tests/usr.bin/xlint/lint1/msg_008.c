/*	$NetBSD: msg_008.c,v 1.4.4.1 2025/08/02 05:58:15 perseant Exp $	*/
# 3 "msg_008.c"

// Test for message: invalid storage class [8]

typedef void
example(void)
/* expect+1: error: invalid storage class [8] */
{
}
