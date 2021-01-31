/*	$NetBSD: msg_232.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_232.c"

// Test for message: label %s unused in function %s [232]

void
example(void)
{
	goto used_label;
unused_label:			/* expect: 232 */
	return;
used_label:
	return;
}
