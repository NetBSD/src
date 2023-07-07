/*	$NetBSD: msg_202.c,v 1.4 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_202.c"

// Test for message: duplicate default in switch [202]

/* lint1-extra-flags: -X 351 */

void
example(int x)
{
	switch (x) {
	case 1:
		break;
	default:
		break;
	default:
		/* expect-1: error: duplicate default in switch [202] */
		return;
	}
}
