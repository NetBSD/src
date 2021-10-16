/* $NetBSD: opt_bs.c,v 1.3 2021/10/16 21:32:10 rillig Exp $ */
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
