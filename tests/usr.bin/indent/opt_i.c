/* $NetBSD: opt_i.c,v 1.3 2022/04/22 21:21:20 rillig Exp $ */

/*
 * Tests for the option '-i', which specifies the indentation for a single
 * brace level.
 */

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
