/*	$NetBSD: msg_063.c,v 1.4 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_063.c"

// Test for message: prototype does not match old-style definition [63]

/* lint1-extra-flags: -X 351 */

int
function(a, b)
	int a, b;
{
	return a + b;
}

/* expect+1: error: prototype does not match old-style definition [63] */
double function(int);
