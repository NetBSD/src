/*	$NetBSD: msg_063.c,v 1.5 2024/12/01 18:37:54 rillig Exp $	*/
# 3 "msg_063.c"

// Test for message: prototype does not match old-style definition [63]

/* lint1-extra-flags: -X 351 */

int
/* expect+1: warning: function definition with identifier list is obsolete in C23 [384] */
function(a, b)
	int a, b;
{
	return a + b;
}

/* expect+1: error: prototype does not match old-style definition [63] */
double function(int);
