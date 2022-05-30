/*	$NetBSD: msg_168.c,v 1.8 2022/05/30 08:51:08 rillig Exp $	*/
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


/*
 * In its expression tree, lint represents pointer addition as 'ptr + off',
 * where 'off' is the offset in bytes, regardless of the pointer type.
 *
 * In the below code, the member 'offset_8' has type 'short', and the
 * expression 's->offset_8' is represented as '&s + 8', or more verbose:
 *
 *	'+' type 'pointer to short'
 *		'&' type 'pointer to struct s'
 *			'name' 's' with auto 'array[1] of struct s', lvalue
 *		'constant' type 'long', value 8
 *
 * The constant 8 differs from the usual model of pointer arithmetics.  Since
 * the type of the '&' expression is 'pointer to struct s', adding a constant
 * would rather be interpreted as adding 'constant * sizeof(struct s)', and
 * to access a member, the pointer to 'struct s' would need to be converted
 * to 'pointer of byte' first, then adding the offset 8, then converting the
 * pointer to the target type 'pointer to short'.
 *
 * Lint uses the simpler representation, saving a few conversions on the way.
 * Without this pre-multiplied representation, the below code would generate
 * warnings about out-of-bounds array access, starting with offset_1.
 */
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
