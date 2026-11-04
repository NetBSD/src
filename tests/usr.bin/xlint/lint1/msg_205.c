/*	$NetBSD: msg_205.c,v 1.4.2.1 2026/11/04 23:28:27 perseant Exp $	*/
# 3 "msg_205.c"

// Test for message: switch expression must have integral type, not '%s' [205]

/* lint1-extra-flags: -X 351 */

/* ARGSUSED */
void
example(double x)
{
	/* expect+1: error: switch expression must have integral type, not 'double' [205] */
	switch (x) {
	case 0:
		break;
	}
}
