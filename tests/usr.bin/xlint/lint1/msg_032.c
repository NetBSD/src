/*	$NetBSD: msg_032.c,v 1.10 2025/01/03 03:14:47 rillig Exp $	*/
# 3 "msg_032.c"

// Test for message: type of parameter '%s' defaults to 'int' [32]

/* lint1-extra-flags: -X 351 */

/* expect+1: warning: function definition for 'add' with identifier list is obsolete in C23 [384] */
add(a, b, c)
/* expect+4: error: old-style declaration; add 'int' [1] */
/* expect+3: warning: type of parameter 'a' defaults to 'int' [32] */
/* expect+2: warning: type of parameter 'b' defaults to 'int' [32] */
/* expect+1: warning: type of parameter 'c' defaults to 'int' [32] */
{
	return a + b + c;
}
