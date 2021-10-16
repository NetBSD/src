/* $NetBSD: opt_bs.c,v 1.2 2021/10/16 05:40:17 rillig Exp $ */
/* $FreeBSD$ */

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
