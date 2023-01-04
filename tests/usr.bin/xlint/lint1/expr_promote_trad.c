/*	$NetBSD: expr_promote_trad.c,v 1.3 2023/01/04 05:47:19 rillig Exp $	*/
# 3 "expr_promote_trad.c"

/*
 * Test arithmetic promotions in traditional C.
 */

/* lint1-flags: -tw */

sink();

struct arithmetic_types {
	/* _Bool is not available in traditional C */
	char plain_char;
	/* signed char is not available in traditional C */
	unsigned char unsigned_char;
	short signed_short;
	unsigned short unsigned_short;
	int signed_int;
	unsigned int unsigned_int;
	long signed_long;
	unsigned long unsigned_long;
	/* (unsigned) long long is not available in traditional C */
	/* __int128_t is not available in traditional C */
	/* __uint128_t is not available in traditional C */
	float single_floating;
	double double_floating;
	/* long double is not available in traditional C */
	/* _Complex is not available in traditional C */
	enum {
		E
	} enumerator;
};

caller(arg)
	struct arithmetic_types *arg;
{
	/* See expr_promote_trad.exp-ln for the resulting types. */
	sink("",
	    arg->plain_char,		/* gets promoted to 'int' */
	    arg->unsigned_char,		/* gets promoted to 'unsigned int' */
	    arg->signed_short,		/* gets promoted to 'int' */
	    arg->unsigned_short,	/* gets promoted to 'unsigned int' */
	    arg->signed_int,
	    arg->unsigned_int,
	    arg->signed_long,
	    arg->unsigned_long,
	    arg->single_floating,	/* gets promoted to 'double' */
	    arg->double_floating,
	    arg->enumerator);
}

/*
 * XXX: Enumerations may need be promoted to 'int', at least C99 6.3.1.1p2
 * suggests that: "If an int can represent ...".
 */
