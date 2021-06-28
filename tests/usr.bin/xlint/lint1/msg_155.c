/*	$NetBSD: msg_155.c,v 1.7 2021/06/28 11:27:00 rillig Exp $	*/
# 3 "msg_155.c"

// Test for message: passing '%s' to incompatible '%s', arg #%d [155]


void c99_6_7_6_example_a(int);
void c99_6_7_6_example_b(int *);
void c99_6_7_6_example_c(int *[3]);
void c99_6_7_6_example_d(int (*)[3]);
void c99_6_7_6_example_e(int (*)[*]);
// FIXME: assertion "sym->s_type != NULL" failed in declare_argument at decl.c:2436
// void c99_6_7_6_example_f(int *());
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

	/* TODO: C99 says 'function with no parameter specification returning a pointer to int' */
	c99_6_7_6_example_f(arg);	/* expect: function implicitly declared */

	/* expect+1: 'pointer to function(void) returning int' */
	c99_6_7_6_example_g(arg);

	/* expect+1: 'pointer to const pointer to function(unsigned int, ...) returning int' */
	c99_6_7_6_example_h(arg);
}
