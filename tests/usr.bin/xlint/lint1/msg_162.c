/*	$NetBSD: msg_162.c,v 1.3 2021/08/23 17:47:34 rillig Exp $	*/
# 3 "msg_162.c"

// Test for message: comparison of %s with %s, op %s [162]

/* lint1-extra-flags: -hp */

void
left_unsigned(unsigned int ui)
{
	if (ui < -5.0) {
	}

	/* expect+1: warning: comparison of unsigned int with negative constant, op < [162] */
	if (ui < -5) {
	}

	/* expect+1: warning: comparison of unsigned int with 0, op < [162] */
	if (ui < 0) {
	}

	/* expect+1: warning: comparison of unsigned int with 0, op >= [162] */
	if (ui >= 0) {
	}

	/* expect+1: warning: comparison of unsigned int with 0, op <= [162] */
	if (ui <= 0) {
	}
}

void
right_unsigned(unsigned int ui)
{

	if (-5.0 > ui) {
	}

	/* expect+1: warning: comparison of negative constant with unsigned int, op > [162] */
	if (-5 > ui) {
	}

	/* expect+1: warning: comparison of 0 with unsigned int, op > [162] */
	if (0 > ui) {
	}

	/* expect+1: warning: comparison of 0 with unsigned int, op <= [162] */
	if (0 <= ui) {
	}

	/* expect+1: warning: comparison of 0 with unsigned int, op >= [162] */
	if (0 >= ui) {
	}
}
