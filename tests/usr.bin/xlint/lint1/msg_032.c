/*	$NetBSD: msg_032.c,v 1.9 2024/12/01 18:37:54 rillig Exp $	*/
# 3 "msg_032.c"

// Test for message: type of parameter '%s' defaults to 'int' [32]

/* lint1-extra-flags: -X 351 */

/* expect+1: warning: function definition with identifier list is obsolete in C23 [384] */
add(a, b, c)
/* expect+4: error: old-style declaration; add 'int' [1] */
/* expect+3: warning: type of parameter 'a' defaults to 'int' [32] */
/* expect+2: warning: type of parameter 'b' defaults to 'int' [32] */
/* expect+1: warning: type of parameter 'c' defaults to 'int' [32] */
{
	return a + b + c;
}
