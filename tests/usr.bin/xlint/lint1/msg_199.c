/*	$NetBSD: msg_199.c,v 1.5 2024/03/27 19:28:20 rillig Exp $	*/
# 3 "msg_199.c"

// Test for message: duplicate case '%jd' in switch [199]

/* lint1-extra-flags: -X 351 */

void
example(int x)
{
	switch (x) {
	case 3:
		/* expect+1: error: duplicate case '3' in switch [199] */
	case 3:
		break;
	}
}
