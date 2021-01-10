/*	$NetBSD: msg_232.c,v 1.2 2021/01/10 13:54:13 rillig Exp $	*/
# 3 "msg_232.c"

// Test for message: label %s unused in function %s [232]

void
example(void)
{
	goto used_label;
unused_label:
	return;
used_label:
	return;
}
