/*	$NetBSD: msg_162.c,v 1.6 2021/09/05 17:49:55 rillig Exp $	*/
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

	/* before 2021-09-05: comparison of unsigned int with 0, op <= [162] */
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

	/* before 2021-09-05: comparison of 0 with unsigned int, op >= [162] */
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

void take_bool(_Bool);

void
compare_operators(unsigned int x)
{
	/* expect+1: warning: comparison of unsigned int with negative constant, op < [162] */
	take_bool(x < -1);
	/* expect+1: warning: comparison of unsigned int with 0, op < [162] */
	take_bool(x < 0);
	take_bool(x < 1);

	/* expect+1: warning: comparison of unsigned int with negative constant, op <= [162] */
	take_bool(x <= -1);
	/*
	 * Before tree.c 1.379 from 2021-09-05, lint warned about
	 * 'unsigned <= 0' as well as '0 >= unsigned'.  In all cases where
	 * the programmer knows whether the underlying data type is signed or
	 * unsigned, it is clearer to express the same thought as
	 * 'unsigned == 0', but that's a stylistic issue only.
	 *
	 * Removing this particular case of the warning is not expected to
	 * miss any bugs.  The expression 'x <= 0' is equivalent to 'x < 1',
	 * so lint should not warn about it, just as it doesn't warn about
	 * the inverted condition, which is 'x > 0'.
	 */
	/* before 2021-09-05: comparison of unsigned int with 0, op <= [162] */
	take_bool(x <= 0);
	take_bool(x <= 1);

	/* expect+1: warning: comparison of unsigned int with negative constant, op > [162] */
	take_bool(x > -1);
	take_bool(x > 0);
	take_bool(x > 1);

	/* expect+1: warning: comparison of unsigned int with negative constant, op >= [162] */
	take_bool(x >= -1);
	/* expect+1: warning: comparison of unsigned int with 0, op >= [162] */
	take_bool(x >= 0);
	take_bool(x >= 1);

	/* expect+1: warning: comparison of unsigned int with negative constant, op == [162] */
	take_bool(x == -1);
	take_bool(x == 0);
	take_bool(x == 1);

	/* expect+1: warning: comparison of unsigned int with negative constant, op != [162] */
	take_bool(x != -1);
	take_bool(x != 0);
	take_bool(x != 1);
}
