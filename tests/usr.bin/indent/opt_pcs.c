/* $NetBSD: opt_pcs.c,v 1.4 2021/10/24 11:31:37 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for the options '-pcs' and '-npcs'.
 *
 * The option '-pcs' adds a space in a function call expression, between the
 * function name and the opening parenthesis.
 *
 * The option '-npcs' removes any whitespace from a function call expression,
 * between the function name and the opening parenthesis.
 */

#indent input
void
example(void)
{
	function_call();
	function_call (1);
	function_call   (1,2,3);
}
#indent end

#indent run -pcs
void
example(void)
{
	function_call ();
	function_call (1);
	function_call (1, 2, 3);
}
#indent end

#indent run -npcs
void
example(void)
{
	function_call();
	function_call(1);
	function_call(1, 2, 3);
}
#indent end

/*
 * The option '-pcs' also applies to 'sizeof' and 'offsetof', even though
 * these are not functions.
 */
#indent input
int sizeof_type = sizeof   (int);
int sizeof_type = sizeof(int);
int sizeof_expr = sizeof   (0);
int sizeof_expr = sizeof(0);
int sizeof_expr = sizeof   0;

int offset = offsetof(struct s, member);
int offset = offsetof   (struct s, member);
#indent end

#indent run -pcs -di0
int sizeof_type = sizeof (int);
int sizeof_type = sizeof (int);
int sizeof_expr = sizeof (0);
int sizeof_expr = sizeof (0);
int sizeof_expr = sizeof 0;

int offset = offsetof (struct s, member);
int offset = offsetof (struct s, member);
#indent end

#indent run -npcs -di0
int sizeof_type = sizeof(int);
int sizeof_type = sizeof(int);
int sizeof_expr = sizeof(0);
int sizeof_expr = sizeof(0);
int sizeof_expr = sizeof 0;

int offset = offsetof(struct s, member);
int offset = offsetof(struct s, member);
#indent end
