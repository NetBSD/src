/*	$NetBSD: msg_348.c,v 1.5 2022/06/11 11:52:13 rillig Exp $	*/
# 3 "msg_348.c"

// Test for message 348: maximum value %d of '%s' does not match maximum array index %d [348]

/* lint1-extra-flags: -r */

enum color {
	red,
	green,
	/* expect+2: previous declaration of 'blue' [260] */
	/* expect+1: previous declaration of 'blue' [260] */
	blue
};

const char *
color_name(enum color color)
{
	static const char *name[] = {
	    "red",
	    "green",
	    "blue"
	};
	/* No warning since the maximum enum value matches the array size. */
	return name[color];
}

const char *
color_name_too_few(enum color color)
{
	static const char *name[] = {
	    "red",
	    "green"
	};
	/* expect+1: warning: maximum value 2 of 'enum color' does not match maximum array index 1 [348] */
	return name[color];
}

const char *
color_name_too_many(enum color color)
{
	static const char *name[] = {
	    "red",
	    "green",
	    "blue",
	    "black"
	};
	/* expect+1: warning: maximum value 2 of 'enum color' does not match maximum array index 3 [348] */
	return name[color];
}

const char *
color_name_computed_index(enum color color)
{
	static const char *name[] = {
	    "unused",
	    "red",
	    "green",
	    "blue"
	};
	/* No warning since the array index is not a name. */
	return name[color + 1];
}

const char *
color_name_cast_from_int(int c)
{
	static const char *name[] = {
	    "unused",
	    "red",
	    "green",
	    "blue"
	};
	/* No warning since the array index before conversion is not a name. */
	return name[(enum color)(c + 1)];
}

const char *
color_name_explicit_cast_to_int(enum color color)
{
	static const char *name[] = {
	    "red",
	    "green",
	};
	/* No warning due to the explicit cast. */
	return name[(int)color];
}

const char *
color_name_computed_pointer(enum color color, const char *name)
{
	/* No warning since 'name' is not an array. */
	return (&name)[color];
}

extern const char *incomplete_color_name[];

const char *
color_name_incomplete_array(enum color color)
{
	/* No warning since 'incomplete_color_name' is incomplete. */
	return incomplete_color_name[color];
}

enum large {
	/* expect+1: warning: integral constant too large [56] */
	min = -1LL << 40,
	/* expect+1: warning: integral constant too large [56] */
	max = 1LL << 40,
	zero = 0
};

const char *
large_name(enum large large)
{
	static const char *name[] = {
	    "dummy",
	};
	/* No warning since at least 1 enum constant is outside of INT. */
	return name[large];
}

enum color_with_count {
	cc_red,
	cc_green,
	cc_blue,
	cc_num_values
};

const char *
color_with_count_name(enum color_with_count color)
{
	static const char *const name[] = { "red", "green", "blue" };
	/* No warning since the maximum enum constant is a count. */
	return name[color];
}

/*
 * If the last enum constant contains "num" in its name, it is not
 * necessarily the count of the other enum values, it may also be a
 * legitimate application value, therefore don't warn in this case.
 */
const char *
color_with_num(enum color_with_count color)
{
	static const char *const name[] = { "r", "g", "b", "num" };
	/* No warning since the maximum values already match. */
	return name[color];
}

enum color_with_uc_count {
	CC_RED,
	CC_GREEN,
	CC_BLUE,
	CC_NUM_VALUES
};

const char *
color_with_uc_count_name(enum color_with_uc_count color)
{
	static const char *const name[] = { "red", "green", "blue" };
	/* No warning since the maximum enum constant is a count. */
	return name[color];
}
