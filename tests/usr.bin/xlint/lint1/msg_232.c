/*	$NetBSD: msg_232.c,v 1.7 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_232.c"

// Test for message: label '%s' unused in function '%s' [232]

/* lint1-extra-flags: -X 351 */

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
