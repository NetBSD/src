/* $NetBSD: opt_i.c,v 1.1 2021/10/22 20:54:36 rillig Exp $ */
/* $FreeBSD$ */

#indent input
void
example(void)
{
   if (1 > 0) if (2 > 1) return yes; return no;
}
#indent end

#indent run -i3
void
example(void)
{
   if (1 > 0)
      if (2 > 1)
	 return yes;
   return no;
}
#indent end
