/*	$NetBSD: msg_063.c,v 1.3 2022/02/27 20:02:44 rillig Exp $	*/
# 3 "msg_063.c"

// Test for message: prototype does not match old-style definition [63]

int
function(a, b)
	int a, b;
{
	return a + b;
}

/* expect+1: error: prototype does not match old-style definition [63] */
double function(int);
