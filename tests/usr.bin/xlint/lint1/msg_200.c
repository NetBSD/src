/*	$NetBSD: msg_200.c,v 1.5 2023/07/09 11:01:27 rillig Exp $	*/
# 3 "msg_200.c"

// Test for message: duplicate case '%lu' in switch [200]

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
