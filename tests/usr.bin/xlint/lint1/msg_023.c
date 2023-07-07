/*	$NetBSD: msg_023.c,v 1.5 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_023.c"

// Test for message: undefined label '%s' [23]

/* lint1-extra-flags: -X 351 */

void
test(void)
{
	goto defined_label;
defined_label:
	/* expect+1: warning: undefined label 'undefined_label' [23] */
	goto undefined_label;
}
