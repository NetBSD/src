/*	$NetBSD: msg_155.c,v 1.10 2022/04/01 22:28:21 rillig Exp $	*/
# 3 "msg_155.c"

// Test for message: passing '%s' to incompatible '%s', arg #%d [155]


void c99_6_7_6_example_a(int);
void c99_6_7_6_example_b(int *);
void c99_6_7_6_example_c(int *[3]);
void c99_6_7_6_example_d(int (*)[3]);
void c99_6_7_6_example_e(int (*)[*]);
/* Wrong type before decl.c 1.256 from 2022-04-01. */
void c99_6_7_6_example_f(int *());
void c99_6_7_6_example_g(int (*)(void));
void c99_6_7_6_example_h(int (*const[])(unsigned int, ...));

struct incompatible {
	int member;
};

void
provoke_error_messages(struct incompatible arg)
{
	/* expect+1: 'int' */
	c99_6_7_6_example_a(arg);

	/* expect+1: 'pointer to int' */
	c99_6_7_6_example_b(arg);

	/* C99 says 'array[3] of pointer to int', which is close enough. */
	/* expect+1: 'pointer to pointer to int' */
	c99_6_7_6_example_c(arg);

	/* expect+1: 'pointer to array[3] of int' */
	c99_6_7_6_example_d(arg);

	/* expect+1: 'pointer to array[unknown_size] of int' */
	c99_6_7_6_example_e(arg);

	/* Wrong type before decl.c 1.256 from 2022-04-01. */
	/* expect+1: 'pointer to function() returning pointer to int' */
	c99_6_7_6_example_f(arg);

	/* expect+1: 'pointer to function(void) returning int' */
	c99_6_7_6_example_g(arg);

	/* expect+1: 'pointer to const pointer to function(unsigned int, ...) returning int' */
	c99_6_7_6_example_h(arg);
}

extern void sink(struct incompatible);

/*
 * The function type_name has a special case for an enum type that has been
 * implicitly converted to an int.  Such a type is still output as the enum
 * type.
 *
 * XXX: The expressions 'day + 0' and '0 + day' should result in the same
 *  type.
 */
void
type_name_of_enum(void)
{
	enum Day {
		MONDAY
	} day = MONDAY;

	/* expect+1: passing 'enum Day' */
	sink(day);

	/* expect+1: passing 'enum Day' */
	sink(day + 0);

	/* expect+1: passing 'int' */
	sink(0 + day);

	/* expect+1: passing 'int' */
	sink(0);
}
