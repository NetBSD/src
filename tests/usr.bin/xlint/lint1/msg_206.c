/*	$NetBSD: msg_206.c,v 1.4 2021/07/08 18:53:57 rillig Exp $	*/
# 3 "msg_206.c"

// Test for message: enumeration value(s) not handled in switch [206]

/* lint1-extra-flags: -eh */

enum number {
	ONE, TWO, THREE
};

void
test(enum number num)
{
	switch (num) {
	case ONE:
	case TWO:
		break;
	}
	/* expect-1: warning: enumeration value(s) not handled in switch [206] */

	switch (num) {
	case ONE:
	case TWO:
	case THREE:
		break;
	}
}
