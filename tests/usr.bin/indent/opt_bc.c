/* $NetBSD: opt_bc.c,v 1.3 2021/10/16 21:32:10 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for the options '-bc' and '-nbc'.
 *
 * The option '-bc' forces a newline after each comma in a declaration.
 *
 * The option '-nbc' keeps everything as is.
 */

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
