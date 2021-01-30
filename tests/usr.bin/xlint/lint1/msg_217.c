/*	$NetBSD: msg_217.c,v 1.2 2021/01/30 17:02:58 rillig Exp $	*/
# 3 "msg_217.c"

// Test for message: function %s falls off bottom without returning value [217]

int
random(int n)
{
	if (n < 0)
		return -3;
}				/* expect: 217 */
