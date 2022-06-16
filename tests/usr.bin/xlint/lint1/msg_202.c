/*	$NetBSD: msg_202.c,v 1.3 2022/06/16 21:24:41 rillig Exp $	*/
# 3 "msg_202.c"

// Test for message: duplicate default in switch [202]

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
