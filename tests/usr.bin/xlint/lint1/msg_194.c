/*	$NetBSD: msg_194.c,v 1.3 2022/06/16 16:58:36 rillig Exp $	*/
# 3 "msg_194.c"

// Test for message: label %s redefined [194]

void example(void)
{
	int i = 0;
label:
	/* expect-1: warning: label 'label' unused in function 'example' [232] */
	i = 1;
label:
	/* expect-1: error: label label redefined [194] */
	i = 2;
}
