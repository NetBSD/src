/*	$NetBSD: msg_200.c,v 1.3 2021/08/27 20:16:50 rillig Exp $	*/
# 3 "msg_200.c"

// Test for message: duplicate case in switch: %lu [200]

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
