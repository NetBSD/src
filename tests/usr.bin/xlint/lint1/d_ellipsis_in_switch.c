/*	$NetBSD: d_ellipsis_in_switch.c,v 1.4 2021/03/27 13:59:18 rillig Exp $	*/
# 3 "d_ellipsis_in_switch.c"

/* Using a range in a case label is a GCC extension. */

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
