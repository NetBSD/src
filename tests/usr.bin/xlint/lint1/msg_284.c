/*	$NetBSD: msg_284.c,v 1.4 2023/03/28 14:44:35 rillig Exp $	*/
# 3 "msg_284.c"

// Test for message: fallthrough on default statement [284]

/* lint1-extra-flags: -h -X 351 */

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
