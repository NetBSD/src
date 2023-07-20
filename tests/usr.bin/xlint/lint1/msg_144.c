/*	$NetBSD: msg_144.c,v 1.6 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_144.c"

// Test for message: cannot take size/alignment of function type '%s' [144]

/* lint1-extra-flags: -X 351 */

unsigned long
example(void)
{
	/* expect+1: error: cannot take size/alignment of function type 'function(void) returning unsigned long' [144] */
	return sizeof(example);
}
