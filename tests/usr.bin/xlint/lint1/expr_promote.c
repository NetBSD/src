/*	$NetBSD: expr_promote.c,v 1.3 2023/01/04 05:47:19 rillig Exp $	*/
# 3 "expr_promote.c"

/*
 * Test arithmetic promotions in C90 and later.
 */

/* lint1-flags: -Sw */

void sink(const char *, ...);

struct arithmetic_types {
	_Bool boolean;
	char plain_char;
	signed char signed_char;
	unsigned char unsigned_char;
	short signed_short;
	unsigned short unsigned_short;
	int signed_int;
	unsigned int unsigned_int;
	long signed_long;
	unsigned long unsigned_long;
	long long signed_long_long;
	unsigned long long unsigned_long_long;
	float float_floating;
	double double_floating;
	long double long_floating;
	float _Complex float_complex;
	double _Complex double_complex;
	long double _Complex long_double_complex;
	enum {
		E
	} enumerator;
};

void
caller(struct arithmetic_types *arg)
{
	/* See expr_promote.exp-ln for the resulting types. */
	sink("",
	    arg->boolean,		/* gets promoted to 'int' */
	    arg->plain_char,		/* gets promoted to 'int' */
	    arg->signed_char,		/* gets promoted to 'int' */
	    arg->unsigned_char,		/* gets promoted to 'int' */
	    arg->signed_short,		/* gets promoted to 'int' */
	    arg->unsigned_short,	/* gets promoted to 'int' */
	    arg->signed_int,
	    arg->unsigned_int,
	    arg->signed_long,
	    arg->unsigned_long,
	    arg->signed_long_long,
	    arg->unsigned_long_long,
	    arg->float_floating,	/* gets promoted to 'double' */
	    arg->double_floating,
	    arg->long_floating,
	    arg->float_complex,
	    arg->double_complex,
	    arg->long_double_complex,
	    arg->enumerator);
}

/*
 * XXX: _Bool should be promoted to 'int', C99 6.3.1.1p2 "If an int can
 * represent ...".
 */
/*
 * XXX: Enumerations may need be promoted to 'int', at least C99 6.3.1.1p2
 * suggests that: "If an int can represent ...".
 */
