/*	$NetBSD: msg_059.c,v 1.5 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_059.c"

// Test for message: formal parameter #%d lacks name [59]

/* lint1-extra-flags: -X 351 */

/* expect+4: error: formal parameter #2 lacks name [59] */
/* expect+3: error: formal parameter #3 lacks name [59] */
int
function_definition(int a, int, double)
{
	return a;
}
