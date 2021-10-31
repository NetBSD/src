/*	$NetBSD: fmt_decl.c,v 1.12 2021/10/31 19:20:53 rillig Exp $	*/
/* $FreeBSD: head/usr.bin/indent/tests/declarations.0 334478 2018-06-01 09:41:15Z pstef $ */

/* See FreeBSD r303570 */

#indent input
typedef void	(*voidptr) (int *);
#indent end

#indent run
typedef void (*voidptr)(int *);
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
 * As of 2021-10-31, indent parses the following function definition as these
 * tokens:
 *
 * line 1: type_at_paren_level_0 type "void"
 * line 1: newline "\n"
 * line 2: funcname "t1"
 * line 2: newline "\n"		repeated, see search_stmt
 * line 3: funcname "t1"	XXX: wrong line_no
 * line 3: lparen_or_lbracket "("
 * line 3: ident type "char"
 * line 3: unary_op "*"
 * line 3: ident "a"
 * line 3: comma ","
 * line 3: ident type "int"
 * line 3: ident "b"
 * line 3: comma ","
 * line 3: newline "\n"
 * line 4: ident type "void"
 * line 4: lparen_or_lbracket "("
 * line 4: unary_op "*"
 * line 4: ident "fn"
 * line 4: rparen_or_rbracket ")"
 * line 4: lparen_or_lbracket "("
 * line 4: ident type "void"
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
int *int_create(void)
{

}
#indent end

#indent run
int	       *
int_create(void)
{

}
#indent end


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
static int ald_shutingdown = 0;
struct thread *ald_thread;
#indent end

#indent run
static LIST_HEAD(, alq) ald_active;
static int	ald_shutingdown = 0;
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
