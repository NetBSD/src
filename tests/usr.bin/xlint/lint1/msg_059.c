/*	$NetBSD: msg_059.c,v 1.3 2022/02/27 20:02:44 rillig Exp $	*/
# 3 "msg_059.c"

// Test for message: formal parameter lacks name: param #%d [59]

/* expect+4: error: formal parameter lacks name: param #2 [59] */
/* expect+3: error: formal parameter lacks name: param #3 [59] */
int
function_definition(int a, int, double)
{
	return a;
}
