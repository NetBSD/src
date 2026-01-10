/*	$NetBSD: msg_205.c,v 1.5 2026/01/10 19:50:40 rillig Exp $	*/
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
