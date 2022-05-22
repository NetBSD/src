/*	$NetBSD: msg_206.c,v 1.5 2022/05/22 13:53:39 rillig Exp $	*/
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

int
too_many(enum number num)
{
	switch (num) {
	case ONE:
		return 1;
	case TWO:
		return 2;
	case THREE:
		return 3;
	case 3:
		return -1;
	}
	/* FIXME: There are _too many_ branches, not _too few_. */
	/* expect-2: warning: enumeration value(s) not handled in switch [206] */
	return 3;
}
