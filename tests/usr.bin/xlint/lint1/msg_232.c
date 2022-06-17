/*	$NetBSD: msg_232.c,v 1.6 2022/06/17 18:54:53 rillig Exp $	*/
# 3 "msg_232.c"

// Test for message: label '%s' unused in function '%s' [232]

void
example(void)
{
	goto used_label;
	/* expect+1: warning: label 'unused_label' unused in function 'example' [232] */
unused_label:
	return;
used_label:
	return;
}
