/* $NetBSD: opt_bc.c,v 1.2 2021/10/16 05:40:17 rillig Exp $ */
/* $FreeBSD$ */

#indent input
int a,b,c;
void function_declaration(int a,int b,int c);
#indent end

#indent run -bc
int		a,
		b,
		c;
void		function_declaration(int a, int b, int c);
#indent end

#indent run -nbc
int		a, b, c;
void		function_declaration(int a, int b, int c);
#indent end
