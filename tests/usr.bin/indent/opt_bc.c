/* $NetBSD: opt_bc.c,v 1.6 2022/04/24 09:04:12 rillig Exp $ */

/*
 * Tests for the options '-bc' and '-nbc'.
 *
 * The option '-bc' forces a newline after each comma in a declaration.
 *
 * The option '-nbc' removes line breaks between declarators.  In most other
 * places, indent preserves line breaks.
 */

//indent input
int a,b,c;
void function_declaration(int a,int b,int c);
int m1,
m2,
m3;
char plain, *pointer;
//indent end

//indent run -bc
int		a,
		b,
		c;
void		function_declaration(int a, int b, int c);
int		m1,
		m2,
		m3;
char		plain,
	       *pointer;
//indent end

//indent run -nbc
int		a, b, c;
void		function_declaration(int a, int b, int c);
int		m1, m2, m3;
char		plain, *pointer;
//indent end


//indent input
old_style_definition(a, b, c)
double a,b,c;
{
    return a+b+c;
}
//indent end

//indent run -bc
old_style_definition(a, b, c)
double		a,
		b,
		c;
{
	return a + b + c;
}
//indent end

//indent run -nbc
old_style_definition(a, b, c)
double		a, b, c;
{
	return a + b + c;
}
//indent end
