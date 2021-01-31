/*	$NetBSD: d_ellipsis_in_switch.c,v 1.2 2021/01/31 14:39:31 rillig Exp $	*/
# 3 "d_ellipsis_in_switch.c"

int x(void)
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
