/* $NetBSD: opt_bc.c,v 1.13 2023/06/17 22:09:24 rillig Exp $ */

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


/*
 * Before indent.c 1.311 from 2023-06-02, indent formatted the two '#if'
 * branches differently and merged the 'b, c' with the preceding preprocessing
 * line.
 */
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
		b,
		c;
int		d;
#endif
//indent end

//indent run -nbc
int		a,
#if 0
		b, c;
int		d;
#else
		b, c;
int		d;
#endif
//indent end


/*
 * Before 2023-06-10, a '(' at the top level started a function definition,
 * leaving variable declaration mode.
 */
//indent input
int a = 1, b = 2;
int a = (1), b = 2;
//indent end

//indent run -bc
int		a = 1,
		b = 2;
int		a = (1),
		b = 2;
//indent end


//indent input
int a,
b,
c;
//indent end

//indent run -nbc -di0
int a, b, c;
//indent end


/*
 * When declarations are too long to fit in a single line, they should not be
 * joined.
 */
//indent input
{
	const struct paren_level *prev = state.prev_ps.paren.item,
	    *curr = ps.paren.item;
}
//indent end

//indent run
// $ FIXME: The line becomes too long.
{
	const struct paren_level *prev = state.prev_ps.paren.item, *curr = ps.paren.item;
}
//indent end


/*
 * In struct or union declarations, the declarators are split onto separate
 * lines, just like in ordinary declarations.
 *
 * In enum declarations and in initializers, no line breaks are added or
 * removed.
 */
//indent input
struct triple_struct {
	int a, b, c;
};
union triple_union {
	int a, b, c;
};
enum triple_enum {
	triple_a, triple_b,

	triple_c,
};
//indent end

//indent run -bc
struct triple_struct {
	int		a,
			b,
			c;
};
union triple_union {
	int		a,
			b,
			c;
};
enum triple_enum {
	triple_a, triple_b,

	triple_c,
};
//indent end

//indent run -nbc
struct triple_struct {
	int		a, b, c;
};
union triple_union {
	int		a, b, c;
};
enum triple_enum {
	triple_a, triple_b,

	triple_c,
};
//indent end
