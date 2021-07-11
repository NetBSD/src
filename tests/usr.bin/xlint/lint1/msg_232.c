/*	$NetBSD: msg_232.c,v 1.5 2021/07/11 19:30:56 rillig Exp $	*/
# 3 "msg_232.c"

// Test for message: label '%s' unused in function '%s' [232]

void
example(void)
{
	goto used_label;
	/* expect+1: label 'unused_label' unused in function 'example' [232] */
unused_label:
	return;
used_label:
	return;
}
