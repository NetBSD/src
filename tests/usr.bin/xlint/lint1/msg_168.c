/*	$NetBSD: msg_168.c,v 1.7 2022/05/30 08:14:53 rillig Exp $	*/
# 3 "msg_168.c"

// Test for message: array subscript cannot be > %d: %ld [168]

void print_string(const char *);
void print_char(char);

void
example(void)
{
	char buf[20] = {};	/* empty initializer is a GCC extension */

	print_string(buf + 19);	/* inside the array */

	/*
	 * It is valid to point at the end of the array, but reading a
	 * character from there invokes undefined behavior.
	 *
	 * The pointer to the end of the array is typically used in (begin,
	 * end) tuples.  These are more common in C++ than in C though.
	 */
	print_string(buf + 20);

	print_string(buf + 21);	/* undefined behavior, not detected */

	print_char(buf[19]);
	print_char(buf[20]);	/* expect: 168 */
}

void
array_with_c99_initializer(void)
{
	static const char *const to_roman[] = {
	    ['0'] = "undefined",
	    ['5'] = "V",
	    ['9'] = "IX"
	};

	print_string(to_roman['9']);
	print_string(to_roman[':']);	/* expect: 168 */
}


struct s {
	char offset_0;
	char offset_1;
	int offset_4;
	short offset_8;
	char offset_10;
};

struct s
s_init(void)
{
	struct s s[1];
	s->offset_0 = 1;
	s->offset_1 = 2;
	s->offset_4 = 3;
	s->offset_8 = 4;
	s->offset_10 = 5;
	return s[0];
}
