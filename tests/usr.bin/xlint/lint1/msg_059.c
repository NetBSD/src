/*	$NetBSD: msg_059.c,v 1.4 2022/06/20 21:13:36 rillig Exp $	*/
# 3 "msg_059.c"

// Test for message: formal parameter #%d lacks name [59]

/* expect+4: error: formal parameter #2 lacks name [59] */
/* expect+3: error: formal parameter #3 lacks name [59] */
int
function_definition(int a, int, double)
{
	return a;
}
