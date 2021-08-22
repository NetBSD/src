/*	$NetBSD: msg_203.c,v 1.3 2021/08/22 13:52:19 rillig Exp $	*/
# 3 "msg_203.c"

/* Test for message: case label must be of type 'int' in traditional C [203] */

/* lint1-flags: -tw */

example(x)
	int x;
{
	switch (x) {
	case (char)3:
		break;
	/* expect+1: warning: case label must be of type 'int' in traditional C [203] */
	case 4L:
		break;
	}
}
