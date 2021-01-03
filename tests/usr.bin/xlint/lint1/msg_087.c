/*	$NetBSD: msg_087.c,v 1.2 2021/01/03 15:35:00 rillig Exp $	*/
# 3 "msg_087.c"

// Test for message: static hides external declaration: %s [87]

/* lint1-flags: -g -h -S -w */

extern int counter;

int
count(void)
{
	static int counter;
	return counter++;
}
