/* $NetBSD: opt_cs.c,v 1.3 2021/10/16 21:32:10 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for the options '-cs' and '-ncs'.
 *
 * The option '-cs' forces a space after the parentheses of a cast.
 *
 * The option '-ncs' removes all whitespace after the parentheses of a cast.
 */

#indent input
int		i0 = (int)3.0;
int		i1 = (int) 3.0;
int		i3 = (int)   3.0;
#indent end

#indent run -cs
int		i0 = (int) 3.0;
int		i1 = (int) 3.0;
int		i3 = (int) 3.0;
#indent end

#indent run -ncs
int		i0 = (int)3.0;
int		i1 = (int)3.0;
int		i3 = (int)3.0;
#indent end
