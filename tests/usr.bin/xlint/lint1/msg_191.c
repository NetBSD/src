/*	$NetBSD: msg_191.c,v 1.4 2022/06/16 16:58:36 rillig Exp $	*/
# 3 "msg_191.c"

// Test for message: '%s' set but not used in function '%s' [191]

void
example(void)
{
	int local;

	/* expect+1: warning: 'local' set but not used in function 'example' [191] */
	local = 3;

	local = 5;
}
