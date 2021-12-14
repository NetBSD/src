/*	$NetBSD: msg_102.c,v 1.3 2021/12/14 18:26:39 rillig Exp $	*/
# 3 "msg_102.c"

// Test for message: illegal member use: %s [102]

// Anonymous members are defined in C11 6.7.2.1p2.

struct bit_fields_and_bits {
	union {
		struct {
			unsigned bit_0:1;
			unsigned bit_1:1;
		};
		unsigned bits;
	};
};

static inline _Bool
eq(const struct bit_fields_and_bits *a, const struct bit_fields_and_bits *b)
{
	/*
	 * TODO: Once this is fixed, enable lint in
	 * external/mit/xorg/lib/dri.old/Makefile again.
	 */
	/* TODO: Add support for C11 anonymous struct and union members. */
	/* expect+2: error: illegal member use: bits [102] */
	/* expect+1: error: illegal member use: bits [102] */
	return a->bits == b->bits;
}
