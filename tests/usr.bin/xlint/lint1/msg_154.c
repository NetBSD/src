/*	$NetBSD: msg_154.c,v 1.6.2.1 2025/08/02 05:58:16 perseant Exp $	*/
# 3 "msg_154.c"

// Test for message: invalid combination of %s '%s' and %s '%s', arg #%d [154]

/* lint1-extra-flags: -X 351 */

void sink_int(int);

void
example(int *ptr)
{
	/* expect+1: warning: invalid combination of integer 'int' and pointer 'pointer to int', arg #1 [154] */
	sink_int(ptr);
}
