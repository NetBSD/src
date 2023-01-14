/*	$NetBSD: msg_348.c,v 1.8 2023/01/14 11:15:07 rillig Exp $	*/
# 3 "msg_348.c"

// Test for message 348: maximum value %d of '%s' does not match maximum array index %d [348]

/* lint1-extra-flags: -r */

enum color {
	red,
	green,
	/* expect+5: previous declaration of 'blue' [260] */
	/* expect+4: previous declaration of 'blue' [260] */
	/* expect+3: previous declaration of 'blue' [260] */
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
	/* No warning since the array index is not a plain identifier. */
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
	/*
	 * No warning since the array index before conversion is not a plain
	 * identifier.
	 */
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
	/*
	 * No warning since the first operand of the selection expression
	 * is '(&name)', whose type is not an array but instead a
	 * 'pointer to pointer to const char'.
	 */
	return (&name)[color];
}

/*
 * If the accessed array has character type, it may contain a trailing null
 * character.
 */
void
color_initial_letter(enum color color)
{
	static const char len_2_null[] = "RG";
	static const char len_3_null[] = "RGB";
	static const char len_4_null[] = "RGB_";

	static const char len_2_of_3[3] = "RG";
	static const char len_3_of_3[3] = "RGB";
	static const char len_4_of_4[4] = "RGB_";

	/* TODO: array is too short */
	if (len_2_null[color] != '\0')
		return;

	/* FIXME: lint should not warn since the maximum usable array index is 2 */
	/* expect+1: warning: maximum value 2 of 'enum color' does not match maximum array index 3 [348] */
	if (len_3_null[color] != '\0')
		return;

	/* FIXME: lint should not warn since the maximum usable array index is 3, not 4 */
	/* expect+1: warning: maximum value 2 of 'enum color' does not match maximum array index 4 [348] */
	if (len_4_null[color] != '\0')
		return;

	/*
	 * The array has 3 elements, as expected.  If lint were to inspect
	 * the content of the array, it could see that [2] is a null
	 * character.  That null character may be intended though.
	 */
	if (len_2_of_3[color] != '\0')
		return;

	if (len_3_of_3[color] != '\0')
		return;

	/* expect+1: warning: maximum value 2 of 'enum color' does not match maximum array index 3 [348] */
	if (len_4_of_4[color])
		return;
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
	/*
	 * No warning since the word 'num' in the last enum constant
	 * MAY indicate a convenience constant for the total number of
	 * values, instead of a regular enum value.
	 */
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

enum uppercase_max {
	M_FIRST,
	M_SECOND,
	M_MAX
};

const char *
uppercase_max_name(enum uppercase_max x)
{
	static const char *const name[] = { "first", "second" };
	return name[x];
}

enum lowercase_max {
	M_first,
	M_second,
	M_max
};

const char *
lowercase_max_name(enum lowercase_max x)
{
	static const char *const name[] = { "first", "second" };
	return name[x];
}
