/*	$NetBSD: msg_130.c,v 1.9 2021/03/09 23:09:48 rillig Exp $	*/
# 3 "msg_130.c"

// Test for message: enum type mismatch: '%s' '%s' '%s' [130]

/* See also msg_241.c, which covers unusual operators on enums. */

enum color {
	RED	= 1 << 0,
	GREEN	= 1 << 1,
	BLUE	= 1 << 2
};

enum size {
	SMALL,
	MEDIUM,
	LARGE
};

enum daytime {
	NIGHT, MORNING, NOON, EVENING
};

void sink(_Bool);

void
example(_Bool cond, enum color c, enum size s)
{
	sink(cond ? GREEN : MORNING);	/* expect: 130 */

	sink(c != s);			/* expect: 130 */
	sink(c == s);			/* expect: 130 */
	sink((c & MEDIUM) != 0);	/* might be useful to warn about */
	sink((c | MEDIUM) != 0);	/* might be useful to warn about */

	c |= MEDIUM;			/* might be useful to warn about */
	c &= MEDIUM;			/* might be useful to warn about */

	/* The cast to unsigned is required by GCC at WARNS=6. */
	c &= ~(unsigned)MEDIUM;		/* might be useful to warn about */
}

void
switch_example(enum color c)
{
	switch (c) {
	case EVENING:			/* expect: 130 */
	case LARGE:			/* expect: 130 */
	case 0:				/* expect: 130 */
		sink(1 == 1);
		break;
	default:
		break;
	}
}

/*
 * Unnamed enum types can be used as a container for constants, especially
 * since in C90 and C99, even after the declaration 'static const int x = 3',
 * 'x' is not a constant expression.
 */
enum {
	sizeof_int = sizeof(int),
	sizeof_long = sizeof(long)
};

enum {
	sizeof_uint = sizeof(unsigned int)
};

int
enum_constant_from_unnamed_type(int x)
{
	switch (x) {
	case sizeof_int:		/* expect: 130 *//* FIXME */
		return 1;
	case sizeof_long:		/* expect: 130 *//* FIXME */
		return 2;
	default:
		break;
	}

	if (x == sizeof_int)
		return 4;
	if (x > sizeof_int)
		return 5;

	if (sizeof_int == sizeof_uint)	/* expect: 130 *//* FIXME */
		return 6;

	return 0;
}
