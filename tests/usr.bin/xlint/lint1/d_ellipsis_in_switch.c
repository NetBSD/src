/*	$NetBSD: d_ellipsis_in_switch.c,v 1.5 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "d_ellipsis_in_switch.c"

/* Using a range in a case label is a GCC extension. */

/* lint1-extra-flags: -X 351 */

int
x(void)
{
	int i = 33;
	switch (i) {
	case 1 ... 40:
		break;
	default:
		break;
	}
	return 0;
}
