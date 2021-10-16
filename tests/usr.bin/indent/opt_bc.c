/* $NetBSD: opt_bc.c,v 1.1 2021/10/16 03:20:13 rillig Exp $ */
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

#indent input
int a,b,c;
void function_declaration(int a,int b,int c);
#indent end

#indent run -nbc
int		a, b, c;
void		function_declaration(int a, int b, int c);
#indent end
