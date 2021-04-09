/*	$NetBSD: msg_191.c,v 1.3 2021/04/09 20:12:01 rillig Exp $	*/
# 3 "msg_191.c"

// Test for message: '%s' set but not used in function '%s' [191]

void
example(void)
{
	int local;

	local = 3;		/* expect: 191 */

	local = 5;
}
