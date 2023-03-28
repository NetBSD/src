/*	$NetBSD: msg_029.c,v 1.6 2023/03/28 14:44:34 rillig Exp $	*/
# 3 "msg_029.c"

// Test for message: '%s' was previously declared extern, becomes static [29]

/* lint1-extra-flags: -X 351 */

extern int function(void);

static int function(void)
/* expect+1: warning: 'function' was previously declared extern, becomes static [29] */
{
	return function();
}
