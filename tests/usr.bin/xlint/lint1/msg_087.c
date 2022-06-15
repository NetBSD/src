/*	$NetBSD: msg_087.c,v 1.4 2022/06/15 20:18:31 rillig Exp $	*/
# 3 "msg_087.c"

// Test for message: static hides external declaration: %s [87]

/* lint1-flags: -g -h -S -w */

extern int counter;

int
count(void)
{
	/* expect+1: warning: static hides external declaration: counter [87] */
	static int counter;
	return counter++;
}
