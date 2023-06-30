/*	$NetBSD: msg_102.c,v 1.6 2023/06/30 21:06:18 rillig Exp $	*/
# 3 "msg_102.c"

// Test for message: illegal use of member '%s' [102]

// Anonymous members are defined in C11 6.7.2.1p2.

struct unrelated {
	union {
		struct {
			unsigned bit_0:1;
			unsigned bit_1:1;
		};
		unsigned bits;
	};
};

struct bit_fields_and_bits {
	union {
		struct {
			unsigned bf_bit_0:1;
			unsigned bf_bit_1:1;
		};
		unsigned bf_bits;
	};
};

static struct unrelated *u1, *u2;
static struct bit_fields_and_bits *b1, *b2;

static inline _Bool
eq(int x)
{
	if (x == 0)
		/* Accessing a member from an unnamed struct member. */
		return u1->bits == u2->bits;

	/*
	 * The struct does not have a member named 'bits'.  There's another
	 * struct with a member of that name, and in traditional C, it was
	 * possible but discouraged to access members of other structs via
	 * their plain name.
	 */
	/* expect+2: error: illegal use of member 'bits' [102] */
	/* expect+1: error: illegal use of member 'bits' [102] */
	return b1->bits == b2->bits;
}
