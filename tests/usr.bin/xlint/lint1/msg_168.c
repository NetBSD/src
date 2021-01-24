/*	$NetBSD: msg_168.c,v 1.2 2021/01/24 16:12:45 rillig Exp $	*/
# 3 "msg_168.c"

// Test for message: array subscript cannot be > %d: %ld [168]

void print_string(const char *);
void print_char(char);

void
example(void)
{
	char buf[20] = {};

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
