/*	$NetBSD: msg_191.c,v 1.5 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_191.c"

// Test for message: '%s' set but not used in function '%s' [191]

/* lint1-extra-flags: -X 351 */

void
example(void)
{
	int local;

	/* expect+1: warning: 'local' set but not used in function 'example' [191] */
	local = 3;

	local = 5;
}
