/* $NetBSD: opt_cs.c,v 1.4 2021/11/07 15:44:28 rillig Exp $ */
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


#indent input
struct s	s3 = (struct s)   s;
struct s       *ptr = (struct s *)   s;
union u		u3 = (union u)   u;
enum e		e3 = (enum e)   e;
#indent end

#indent run -cs
struct s	s3 = (struct s) s;
struct s       *ptr = (struct s *) s;
union u		u3 = (union u) u;
enum e		e3 = (enum e) e;
#indent end

#indent run -ncs
struct s	s3 = (struct s)s;
struct s       *ptr = (struct s *)s;
union u		u3 = (union u)u;
enum e		e3 = (enum e)e;
#indent end
