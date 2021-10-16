/* $NetBSD: opt_cs.c,v 1.1 2021/10/16 03:20:13 rillig Exp $ */
/* $FreeBSD$ */

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

#indent input
int		i0 = (int)3.0;
int		i1 = (int) 3.0;
int		i3 = (int)   3.0;
#indent end

#indent run -ncs
int		i0 = (int)3.0;
int		i1 = (int)3.0;
int		i3 = (int)3.0;
#indent end
