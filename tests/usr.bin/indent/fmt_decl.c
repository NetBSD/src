/*	$NetBSD: fmt_decl.c,v 1.22 2021/11/21 11:02:25 rillig Exp $	*/
/* $FreeBSD: head/usr.bin/indent/tests/declarations.0 334478 2018-06-01 09:41:15Z pstef $ */

/*
 * Tests for declarations of global variables, external functions, and local
 * variables.
 *
 * See also:
 *	opt_di.c
 */

/* See FreeBSD r303570 */

/*
 * A type definition usually declares a single type, so there is no need to
 * align the newly declared type name with the other variables.
 */
#indent input
typedef   void   (   *   voidptr   )   (   int   *   )   ;
#indent end

#indent run
typedef void (*voidptr)(int *);
#indent end


/*
 * In variable declarations, the names of the first declarators are indented
 * by the amount given in '-di', which defaults to 16.
 */
#indent input
extern   void   (   *   function_pointer   )   (   int   *   )   ;
extern   void   *   pointer;
#indent end

#indent run
/* $ XXX: Why is the token 'function_pointer' not aligned with 'pointer'? */
extern void	(*function_pointer)(int *);
extern void    *pointer;
#indent end


#indent input
static const struct
{
	double		x;
	double		y, z;
} n[m + 1] =
{
	{
		.0,
		.9,
		5
	}
};
#indent end

#indent run
static const struct {
	double		x;
	double		y, z;
}		n[m + 1] =
{
	{
		.0,
		.9,
		5
	}
};
#indent end


#indent input
typedef struct Complex
{
	double		x;
	double		y;
}	Complex;
#indent end

#indent run
typedef struct Complex {
	double		x;
	double		y;
}		Complex;
#indent end


/*
 * As of 2021-11-07, indent parses the following function definition as these
 * tokens:
 *
 * line 1: type_outside_parentheses "void"
 * line 1: newline "\n"
 * line 2: funcname "t1"
 * line 2: newline "\n"		repeated, see search_stmt
 * line 3: funcname "t1"	XXX: wrong line_no
 * line 3: lparen_or_lbracket "("
 * line 3: type_in_parentheses "char"
 * line 3: unary_op "*"
 * line 3: word "a"
 * line 3: comma ","
 * line 3: type_in_parentheses "int"
 * line 3: word "b"
 * line 3: comma ","
 * line 3: newline "\n"
 * line 4: type_in_parentheses "void"
 * line 4: lparen_or_lbracket "("
 * line 4: unary_op "*"
 * line 4: word "fn"
 * line 4: rparen_or_rbracket ")"
 * line 4: lparen_or_lbracket "("
 * line 4: type_in_parentheses "void"
 * line 4: rparen_or_rbracket ")"
 * line 4: rparen_or_rbracket ")"
 * line 4: newline "\n"
 * line 5: lbrace "{"
 * line 5: lbrace "{"		repeated, see search_stmt
 * line 5: newline "\n"		FIXME: there is no newline in the source
 * line 6: rbrace "}"
 * line 6: eof "\n"
 */
#indent input
void
t1 (char *a, int b,
	void (*fn)(void))
{}
#indent end

#indent run
void
t1(char *a, int b,
   void (*fn)(void))
{
}
#indent end


/* See opt_bc.c. */
#indent input
void t2 (char *x, int y)
{
	int a,
	b,
	c;
	int
	*d,
	*e,
	*f;
	int (*g)(),
	(*h)(),
	(*i)();
	int j,
	k,
	l;
	int m
	,n
	,o
	;
	int		chars[ /* push the comma beyond column 74 .... */ ], x;
}
#indent end

#indent run
void
t2(char *x, int y)
{
	int		a, b, c;
	int
		       *d, *e, *f;
	int		(*g)(), (*h)(), (*i)();
	int		j, k, l;
	int		m
		       ,n
		       ,o
		       ;
	int		chars[ /* push the comma beyond column 74 .... */ ],
			x;
}
#indent end


#indent input
const int	int_minimum_size =
MAXALIGN(offsetof(int, test)) + MAXIMUM_ALIGNOF;
#indent end

#indent run-equals-input


#indent input
static
_attribute_printf(1, 2)
void
print_error(const char *fmt,...)
{
}
#indent end

#indent run
static
_attribute_printf(1, 2)
void
print_error(const char *fmt, ...)
{
}
#indent end


#indent input
static _attribute_printf(1, 2)
void
print_error(const char *fmt,...)
{
}
#indent end

#indent run
static _attribute_printf(1, 2)
void
print_error(const char *fmt, ...)
{
}
#indent end


#indent input
static void _attribute_printf(1, 2)
print_error(const char *fmt,...)
{
}
#indent end

#indent run
static void
_attribute_printf(1, 2)
print_error(const char *fmt, ...)
{
}
#indent end


/* See FreeBSD r309380 */
#indent input
static LIST_HEAD(, alq) ald_active;
static int ald_shutting_down = 0;
struct thread *ald_thread;
#indent end

#indent run
static LIST_HEAD(, alq) ald_active;
static int	ald_shutting_down = 0;
struct thread  *ald_thread;
#indent end


#indent input
static int
old_style_definition(a, b, c)
	struct thread *a;
	int b;
	double ***c;
{

}
#indent end

#indent run
static int
old_style_definition(a, b, c)
	struct thread  *a;
	int		b;
	double	     ***c;
{

}
#indent end


/*
 * Demonstrate how variable declarations are broken into several lines when
 * the line length limit is set quite low.
 */
#indent input
struct s a,b;
struct s0 a,b;
struct s01 a,b;
struct s012 a,b;
struct s0123 a,b;
struct s01234 a,b;
struct s012345 a,b;
struct s0123456 a,b;
struct s01234567 a,b;
struct s012345678 a,b;
struct s0123456789 a,b;
struct s01234567890 a,b;
struct s012345678901 a,b;
struct s0123456789012 a,b;
struct s01234567890123 a,b;
#indent end

#indent run -l20 -di0
struct s a, b;
/* $ XXX: See process_comma, varname_len for why this line is broken. */
struct s0 a,
   b;
/* $ XXX: The indentation of the second line is wrong. The variable names */
/* $ XXX: 'a' and 'b' should be in the same column; the word 'struct' is */
/* $ XXX: missing in the calculation for the indentation. */
struct s01 a,
    b;
struct s012 a,
     b;
struct s0123 a,
      b;
struct s01234 a,
       b;
struct s012345 a,
        b;
struct s0123456 a,
         b;
struct s01234567 a,
          b;
struct s012345678 a,
           b;
struct s0123456789 a,
            b;
struct s01234567890 a,
             b;
struct s012345678901 a,
              b;
struct s0123456789012 a,
               b;
struct s01234567890123 a,
                b;
#indent end


#indent input
char * x(void)
{
    type identifier;
    type *pointer;
    unused * value;
    (void)unused * value;

    dmax = (double)3 * 10.0;
    dmin = (double)dmax * 10.0;
    davg = (double)dmax * dmin;

    return NULL;
}
#indent end

#indent run
char *
x(void)
{
	type		identifier;
	type	       *pointer;
	unused	       *value;
	(void)unused * value;

	dmax = (double)3 * 10.0;
	dmin = (double)dmax * 10.0;
	davg = (double)dmax * dmin;

	return NULL;
}
#indent end


#indent input
int *
y(void) {

}

int
z(void) {

}
#indent end

#indent run
int *
y(void)
{

}

int
z(void)
{

}
#indent end


#indent input
int x;
int *y;
int * * * * z;
#indent end

#indent run
int		x;
int	       *y;
int	    ****z;
#indent end


#indent input
int main(void) {
    char (*f1)() = NULL;
    char *(*f1)() = NULL;
    char *(*f2)();
}
#indent end

/*
 * Before NetBSD io.c 1.103 from 2021-10-27, indent wrongly placed the second
 * and third variable declaration in column 1. This bug has been introduced
 * to NetBSD when FreeBSD indent was imported in 2019.
 */
#indent run -ldi0
int
main(void)
{
	char (*f1)() = NULL;
	char *(*f1)() = NULL;
	char *(*f2)();
}
#indent end

#indent run
int
main(void)
{
/* $ XXX: Not really pretty, the name 'f1' should be aligned, if at all. */
	char		(*f1)() = NULL;
/* $ XXX: Not really pretty, the name 'f1' should be aligned, if at all. */
	char *(*	f1)() = NULL;
/* $ XXX: Not really pretty, the name 'f2' should be aligned, if at all. */
	char *(*	f2)();
}
#indent end


/*
 * In some ancient time long before ISO C90, variable declarations with
 * initializer could be written without '='. The C Programming Language from
 * 1978 doesn't mention this form anymore.
 *
 * Before NetBSD lexi.c 1.123 from 2021-10-31, indent treated the '-' as a
 * unary operator.
 */
#indent input
int a - 1;
{
int a - 1;
}
#indent end

#indent run -di0
int a - 1;
{
	int a - 1;
}
#indent end


/*
 * Between 2019-04-04 and before lexi.c 1.146 from 2021-11-19, the indentation
 * of the '*' depended on the function name, which did not make sense.  For
 * function names that matched [A-Za-z]+, the '*' was placed correctly, for
 * all other function names (containing [$0-9_]) the '*' was right-aligned on
 * the declaration indentation, which defaults to 16.
 */
#indent input
int *
f2(void)
{
}

int *
yy(void)
{
}

int *
int_create(void)
{
}
#indent end

#indent run-equals-input


/*
 * Since 2019-04-04, the space between the '){' is missing.
 */
#indent input
int *
function_name_____20________30________40________50
(void)
{}
#indent end

/* FIXME: The space between '){' is missing. */
#indent run
int	       *function_name_____20________30________40________50
		(void){
}
#indent end


/*
 * Since 2019-04-04 and before lexi.c 1.144 from 2021-11-19, some function
 * names were preserved while others were silently discarded.
 */
#indent input
int *
aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
(void)
{}
#indent end

#indent run
int	       *aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
		(void){
}
#indent end


/*
 * Before 1990, when C90 standardized function prototypes, a function
 * declaration or definition did not contain a '*' that may have looked
 * similar to the binary operator '*' because it was surrounded by two
 * identifiers.
 *
 * As of 2021-11-21, indent interprets the '*' in the function declaration in
 * line 1 as a binary operator, even though it is followed by a ',' directly.
 * In the function declaration in line 2, as well as the function definition
 * in line 4, indent interprets the '*' as a binary operator as well, which
 * kind of makes sense since it is surrounded by words, but it's still in a
 * declaration.
 *
 * Essentially, as of 2021, indent has missed the last 31 years of advances in
 * the C programming language.  Instead, the workaround has been to pass all
 * type names via the options '-ta' and '-T'.
 */
#indent input
void		buffer_add(buffer *, char);
void		buffer_add(buffer *buf, char ch);

void
buffer_add(buffer *buf, char ch)
{
	*buf->e++ = ch;
}
#indent end

#indent run
void		buffer_add(buffer *, char);
void		buffer_add(buffer * buf, char ch);

void
buffer_add(buffer * buf, char ch)
{
	*buf->e++ = ch;
}
#indent end
