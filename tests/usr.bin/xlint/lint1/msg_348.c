/*	$NetBSD: msg_348.c,v 1.1 2021/10/30 22:04:42 rillig Exp $	*/
# 3 "msg_348.c"

// Test for message 348: maximum value %d of '%s' does not match maximum array index %d [348]

enum color {
	red,
	green,
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
