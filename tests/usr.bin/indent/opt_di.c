/* $NetBSD: opt_di.c,v 1.1 2021/10/22 20:54:36 rillig Exp $ */
/* $FreeBSD$ */

#indent input
int space;
int	tab;
int		tab16;

struct long_name long_name;
#indent end

#indent run -di8
int	space;
int	tab;
int	tab16;

struct long_name long_name;
#indent end
