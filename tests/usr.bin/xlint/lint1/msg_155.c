/*	$NetBSD: msg_155.c,v 1.3 2021/06/28 10:07:43 rillig Exp $	*/
# 3 "msg_155.c"

// Test for message: argument is incompatible with prototype, arg #%d [155]
// TODO: Add type information to the message

void c99_6_7_6_example_a(int);
void c99_6_7_6_example_b(int *);
void c99_6_7_6_example_c(int *[3]);
void c99_6_7_6_example_d(int (*)[3]);
void c99_6_7_6_example_e(int (*)[*]);	/* expect: syntax error ']' *//* FIXME */
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
	/* expect+1: argument is incompatible with prototype, arg #1 */
	c99_6_7_6_example_a(arg);

	/* expect+1: argument is incompatible with prototype, arg #1 */
	c99_6_7_6_example_b(arg);

	/* expect+1: argument is incompatible with prototype, arg #1 */
	c99_6_7_6_example_c(arg);

	/* expect+1: argument is incompatible with prototype, arg #1 */
	c99_6_7_6_example_d(arg);

	/* FIXME: no warning or error at all for an undefined function? */
	c99_6_7_6_example_e(arg);

	/* FIXME: no warning or error at all for an undefined function? */
	c99_6_7_6_example_f(arg);

	/* expect+1: argument is incompatible with prototype, arg #1 */
	c99_6_7_6_example_g(arg);

	/* expect+1: argument is incompatible with prototype, arg #1 */
	c99_6_7_6_example_h(arg);
}
