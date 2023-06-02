/* $NetBSD: opt_bc.c,v 1.7 2023/06/02 11:26:21 rillig Exp $ */

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


//indent input
int a,
#if 0
b, c; int d;
#else
b, c; int d;
#endif
//indent end

//indent run -bc
int		a,
#if 0
		b,
		c;
int		d;
#else
// $ FIXME: The '#else' branch must be indented like the '#if' branch.
		b, c;
int		d;
#endif
//indent end

//indent run -nbc
int		a,
// $ FIXME: 'b, c' must not be merged into the preprocessing line.
#if 0		b, c;
int		d;
#else
		b, c;
int		d;
#endif
//indent end
