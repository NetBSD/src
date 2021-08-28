/*	$NetBSD: msg_162.c,v 1.4 2021/08/28 14:45:19 rillig Exp $	*/
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

/*
 * Lint does not care about these comparisons, even though they are obviously
 * out of range.
 */
void
compare_signed_char(signed char sc)
{
	if (sc == -129)
		return;
	if (sc == -128)
		return;
	if (sc == 127)
		return;
	if (sc == 128)
		return;
}

void
compare_unsigned_char(unsigned char uc)
{
	/* expect+1: warning: comparison of unsigned char with negative constant, op == [162] */
	if (uc == -1)
		return;
	if (uc == 0)
		return;
	if (uc == 255)
		return;
	if (uc == 256)
		return;
}
