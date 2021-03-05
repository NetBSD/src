/*	$NetBSD: msg_130.c,v 1.8 2021/03/05 17:10:06 rillig Exp $	*/
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
