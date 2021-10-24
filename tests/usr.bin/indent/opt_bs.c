/* $NetBSD: opt_bs.c,v 1.4 2021/10/24 11:42:57 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for the options '-bs' and '-nbs'.
 *
 * The option '-bs' forces a space after the keyword 'sizeof'.
 *
 * The option '-nbs' removes all whitespace after the keyword 'sizeof', unless
 * the next token is a word as well.
 */

#indent input
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
#indent end

#indent run -bs
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
#indent end

#indent run -nbs
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
#indent end

/*
 * The option '-bs' only affects 'sizeof', not 'offsetof', even though these
 * two keywords are syntactically similar.
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
