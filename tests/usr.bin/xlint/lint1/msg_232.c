/*	$NetBSD: msg_232.c,v 1.4 2021/07/11 19:24:42 rillig Exp $	*/
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

/* TODO: add quotes around '%s' */
