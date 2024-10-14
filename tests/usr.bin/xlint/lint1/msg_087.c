/*	$NetBSD: msg_087.c,v 1.7 2024/10/14 18:43:24 rillig Exp $	*/
# 3 "msg_087.c"

// Test for message: static '%s' hides external declaration with type '%s' [87]

/* lint1-flags: -g -h -S -w -X 351 */

extern long counter;

int
count(void)
{
	/* expect+1: warning: static 'counter' hides external declaration with type 'long' [87] */
	static int counter;
	return counter++;
}
