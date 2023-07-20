/*	$NetBSD: msg_011.c,v 1.7 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_011.c"

// Test for message: bit-field initializer out of range [11]

/* lint1-extra-flags: -X 351 */

void
example(void)
{
	struct {
		signed int si: 3;
		unsigned int ui: 3;
	/* expect+1: warning: 's' set but not used in function 'example' [191] */
	} s[] = {
		/* expect+2: warning: bit-field initializer out of range [11] */
		/* expect+1: warning: initialization of unsigned with negative constant [221] */
		{ -8, -8 },

		/* expect+2: warning: bit-field initializer out of range [11] */
		/* expect+1: warning: initialization of unsigned with negative constant [221] */
		{ -7, -7 },

		/* expect+1: warning: initialization of unsigned with negative constant [221] */
		{ -4, -4 },

		/* expect+1: warning: initialization of unsigned with negative constant [221] */
		{ -3, -3 },

		{ 3, 3 },

		/* expect+1: warning: bit-field initializer out of range [11] */
		{ 4, 4 },

		/* expect+1: warning: bit-field initializer out of range [11] */
		{ 7, 7 },

		/* expect+2: warning: bit-field initializer does not fit [180] */
		/* expect+1: warning: bit-field initializer does not fit [180] */
		{ 8, 8 },
	};
}
