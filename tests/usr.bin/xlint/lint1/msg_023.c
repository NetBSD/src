/*	$NetBSD: msg_023.c,v 1.4 2021/07/11 19:30:56 rillig Exp $	*/
# 3 "msg_023.c"

// Test for message: undefined label '%s' [23]

void
test(void)
{
	goto defined_label;
defined_label:
	/* expect+1: warning: undefined label 'undefined_label' [23] */
	goto undefined_label;
}
