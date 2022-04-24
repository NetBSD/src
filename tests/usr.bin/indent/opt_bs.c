/* $NetBSD: opt_bs.c,v 1.10 2022/04/24 09:04:12 rillig Exp $ */

/*
 * Tests for the options '-bs' and '-nbs' ("blank after sizeof").
 *
 * The option '-bs' forces a space after the keyword 'sizeof'.
 *
 * The option '-nbs' removes horizontal whitespace after the keyword 'sizeof',
 * unless the next token is a word as well.
 */

//indent input
void
example(int i)
{
	print(sizeof(i));
	print(sizeof(int));

	print(sizeof i);
	print(sizeof (i));
	print(sizeof (int));

	print(sizeof   i);
	print(sizeof   (i));
	print(sizeof   (int));
}
//indent end

//indent run -bs
void
example(int i)
{
	print(sizeof (i));
	print(sizeof (int));

	print(sizeof i);
	print(sizeof (i));
	print(sizeof (int));

	print(sizeof i);
	print(sizeof (i));
	print(sizeof (int));
}
//indent end

//indent run -nbs
void
example(int i)
{
	print(sizeof(i));
	print(sizeof(int));

	print(sizeof i);
	print(sizeof(i));
	print(sizeof(int));

	print(sizeof i);
	print(sizeof(i));
	print(sizeof(int));
}
//indent end


/*
 * The option '-bs' only affects 'sizeof', not 'offsetof', even though these
 * two keywords are syntactically similar.
 */
//indent input
int sizeof_type = sizeof   (int);
int sizeof_type = sizeof(int);
int sizeof_expr = sizeof   (0);
int sizeof_expr = sizeof(0);
int sizeof_expr = sizeof   0;

int offset = offsetof(struct s, member);
int offset = offsetof   (struct s, member);
//indent end

//indent run -pcs -di0
int sizeof_type = sizeof (int);
int sizeof_type = sizeof (int);
int sizeof_expr = sizeof (0);
int sizeof_expr = sizeof (0);
int sizeof_expr = sizeof 0;

int offset = offsetof (struct s, member);
int offset = offsetof (struct s, member);
//indent end

//indent run -npcs -di0
int sizeof_type = sizeof(int);
int sizeof_type = sizeof(int);
int sizeof_expr = sizeof(0);
int sizeof_expr = sizeof(0);
int sizeof_expr = sizeof 0;

int offset = offsetof(struct s, member);
int offset = offsetof(struct s, member);
//indent end


/* Ensure that there is no blank before 'sizeof(' if there is a '\n' between. */
//indent input
int sizeof_newline = sizeof
(0);
//indent end

//indent run-equals-input -di0 -bs

//indent run-equals-input -di0 -nbs


/* Ensure that only the first '(' after 'sizeof' gets a blank. */
//indent input
int sizeof_parenthesized = sizeof((0));
//indent end

//indent run -di0 -bs
int sizeof_parenthesized = sizeof ((0));
//indent end

//indent run-equals-input -di0 -nbs
