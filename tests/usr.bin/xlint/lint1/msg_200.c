/*	$NetBSD: msg_200.c,v 1.6 2024/03/27 19:28:20 rillig Exp $	*/
# 3 "msg_200.c"

// Test for message: duplicate case '%ju' in switch [200]

/* lint1-extra-flags: -X 351 */

void
example(unsigned x)
{
	switch (x) {
	case 3:
	/* expect+1: error: duplicate case '3' in switch [200] */
	case 3:
		break;
	}
}
