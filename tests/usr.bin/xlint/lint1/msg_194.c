/*	$NetBSD: msg_194.c,v 1.5 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_194.c"

// Test for message: label '%s' redefined [194]

/* lint1-extra-flags: -X 351 */

void example(void)
{
	int i = 0;
label:
	/* expect-1: warning: label 'label' unused in function 'example' [232] */
	i = 1;
label:
	/* expect-1: error: label 'label' redefined [194] */
	i = 2;
}
