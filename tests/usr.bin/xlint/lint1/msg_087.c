/*	$NetBSD: msg_087.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_087.c"

// Test for message: static hides external declaration: %s [87]

/* lint1-flags: -g -h -S -w */

extern int counter;

int
count(void)
{
	static int counter;		/* expect: 87 */
	return counter++;
}
