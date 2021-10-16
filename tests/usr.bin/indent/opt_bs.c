/* $NetBSD: opt_bs.c,v 1.1 2021/10/16 03:20:13 rillig Exp $ */
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
