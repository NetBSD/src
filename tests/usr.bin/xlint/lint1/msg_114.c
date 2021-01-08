/*	$NetBSD: msg_114.c,v 1.2 2021/01/08 21:25:03 rillig Exp $	*/
# 3 "msg_114.c"

// Test for message: %soperand of '%s' must be lvalue [114]

void
example(int a)
{
	3++;
	// FIXME: lint error: ../common/tyname.c, 190: tspec_name(0)
	// "string"++;
	(a + a)++;
}
