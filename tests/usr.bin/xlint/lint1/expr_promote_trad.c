/*	$NetBSD: expr_promote_trad.c,v 1.6 2025/04/12 15:57:26 rillig Exp $	*/
# 3 "expr_promote_trad.c"

/*
 * Test arithmetic promotions in traditional C.
 */

/* lint1-flags: -tw -X 351 */

sink();

struct arithmetic_types {
	/* _Bool requires C90 or later */
	char plain_char;
	/* signed char requires C90 or later */
	unsigned char unsigned_char;
	short signed_short;
	unsigned short unsigned_short;
	int signed_int;
	unsigned int unsigned_int;
	long signed_long;
	unsigned long unsigned_long;
	/* (unsigned) long long requires C90 or later */
	/* __int128_t requires C90 or later */
	/* __uint128_t requires C90 or later */
	float single_floating;
	double double_floating;
	/* long double requires C90 or later */
	/* _Complex requires C90 or later */
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
	    arg->enumerator);		/* should get promoted to 'int' */
}

/*
 * XXX: Enumerations may need be promoted to 'int', at least C99 6.3.1.1p2
 * suggests that: "If an int can represent ...".
 */
