/*	$NetBSD: msg_130.c,v 1.15 2022/06/16 16:58:36 rillig Exp $	*/
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
	/* expect+1: warning: enum type mismatch: 'enum color' ':' 'enum daytime' [130] */
	sink(cond ? GREEN : MORNING);
	/* expect+1: warning: enum type mismatch: 'enum color' '!=' 'enum size' [130] */
	sink(c != s);
	/* expect+1: warning: enum type mismatch: 'enum color' '==' 'enum size' [130] */
	sink(c == s);
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
	case EVENING:			/* maybe someday expect: 130 */
	case LARGE:			/* maybe someday expect: 130 */
	case 0:				/* maybe someday expect: 130 */
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
	sizeof_short = sizeof(short)
};

enum {
	sizeof_uint = sizeof(unsigned int)
};

int
enum_constant_from_unnamed_type(int x)
{
	/* using an enum constant as constant-expression */
	switch (x) {
	case sizeof_int:
		return 1;
	case sizeof_short:
		return 2;
	default:
		break;
	}

	if (x == sizeof_int)
		return 4;
	if (x > sizeof_int)
		return 5;

	/* FIXME */
	/* expect+1: warning: enum type mismatch: 'enum <unnamed>' '==' 'enum <unnamed>' [130] */
	if (sizeof_int == sizeof_uint)
		return 6;

	/* expect+1: warning: statement not reached [193] */
	return 0;
}

/*
 * A typical legitimate use case for an anonymous enum type that should not
 * be mixed with other types is a state machine.
 *
 * This example demonstrates that the type of the 'switch' expression can be
 * an anonymous enum.
 */
void
state_machine(const char *str)
{
	enum {
		begin,
		seen_letter,
		seen_letter_digit,
		error
	} state = begin;

	for (const char *p = str; *p != '\0'; p++) {
		switch (state) {
		case begin:
			state = *p == 'A' ? seen_letter : error;
			break;
		case seen_letter:
			state = *p == '1' ? seen_letter_digit : error;
			break;
		default:
			state = error;
		}
	}

	if (state == 2)			/* might be worth a warning */
		return;
	/* expect+1: warning: enum type mismatch: 'enum <unnamed>' '==' 'enum <unnamed>' [130] */
	if (state == sizeof_int)
		return;
}

/*
 * For check_case_label_enum, a warning only makes sense if the type of the
 * enum can actually be specified somehow, either explicitly by using a tag
 * name or a typedef name, or implicitly by using a variable in a switch
 * expression.
 */

typedef enum {
	has_typedef = 1001
} typedef_name;

enum tag_name {
	has_tag = 1002
};

enum {
	has_variable = 1003
} variable;

enum {
	inaccessible = 1004
};

/*
 * This check is already done by Clang, so it may not be necessary to add it
 * to lint as well.  Except if there are some cases that Clang didn't
 * implement.
 */
void
test_check_case_label_enum(enum color color)
{
	switch (color)
	{
	case has_typedef:
	case has_tag:
	case has_variable:
	case inaccessible:
		return;
	}
}
