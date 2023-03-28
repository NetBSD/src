/*	$NetBSD: msg_087.c,v 1.6 2023/03/28 14:44:34 rillig Exp $	*/
# 3 "msg_087.c"

// Test for message: static '%s' hides external declaration [87]

/* lint1-flags: -g -h -S -w -X 351 */

extern int counter;

int
count(void)
{
	/* expect+1: warning: static 'counter' hides external declaration [87] */
	static int counter;
	return counter++;
}
