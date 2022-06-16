/*	$NetBSD: msg_284.c,v 1.3 2022/06/16 21:24:41 rillig Exp $	*/
# 3 "msg_284.c"

// Test for message: fallthrough on default statement [284]

/* lint1-extra-flags: -h */

void print_int(int);

void
example(int x)
{
	switch (x) {
	case 1:
		print_int(x);
		/* expect+1: warning: fallthrough on default statement [284] */
	default:
		print_int(0);
		/* expect+1: warning: fallthrough on case statement [220] */
	case 2:
		print_int(x);
	}
}
