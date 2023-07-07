/*	$NetBSD: msg_200.c,v 1.4 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_200.c"

// Test for message: duplicate case in switch: %lu [200]

/* lint1-extra-flags: -X 351 */

void
example(unsigned x)
{
	switch (x) {
	case 3:
	/* expect+1: error: duplicate case in switch: 3 [200] */
	case 3:
		break;
	}
}
