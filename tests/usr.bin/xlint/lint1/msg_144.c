/*	$NetBSD: msg_144.c,v 1.5 2022/04/01 23:16:32 rillig Exp $	*/
# 3 "msg_144.c"

// Test for message: cannot take size/alignment of function type '%s' [144]

unsigned long
example(void)
{
	/* expect+1: error: cannot take size/alignment of function type 'function(void) returning unsigned long' [144] */
	return sizeof(example);
}
