/*	$NetBSD: msg_196.c,v 1.4 2023/09/07 06:17:35 rillig Exp $	*/
# 3 "msg_196.c"

// Test for message: case label affected by conversion [196]

/* lint1-extra-flags: -X 351 */

void
switch_int_unsigned(int x)
{
	switch (x) {
		/* expect+1: warning: case label affected by conversion [196] */
	case (unsigned int)-1:
		/* expect+1: warning: case label affected by conversion [196] */
	case -2U:
		/* expect+1: warning: case label affected by conversion [196] */
	case 0x1000200030004000ULL:
		return;
	}
}
