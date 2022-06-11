/*	$NetBSD: gcc_init_compound_literal.c,v 1.5 2022/06/11 11:52:13 rillig Exp $	*/
# 3 "gcc_init_compound_literal.c"

/*
 * C99 says in 6.7.8p4:
 *
 *	All the expressions in an initializer for an object that has static
 *	storage duration shall be constant expressions or string literals.
 *
 * The term "constant expression" is defined in C99 6.6, where 6.6p9 allows
 * "constant expressions" in initializers to also be an "address constant".
 * Using these address constants, it is possible to reference an unnamed
 * object created by a compound literal (C99 6.5.2.5), using either an
 * explicit '&' or the implicit array-to-pointer conversion from C99 6.3.2.1.
 *
 * Before init.c 1.195 from 2021-04-17, lint failed with an assertion failure
 * in check_global_variable, called by check_global_symbols since these
 * temporary objects have neither storage class EXTERN nor STATIC.
 */

// Seen in sys/crypto/aes/aes_ccm.c.
const struct {
    const unsigned char *ctxt;
} T = {
	.ctxt = (const unsigned char[4]){
	    1, 2, 3, 4
	},
};

struct node {
	int num;
	struct node *left;
	struct node *right;
};

/*
 * Initial tree for representing the decisions in the classic number guessing
 * game often used in teaching the basics of programming.
 */
/* expect+1: static variable 'guess' unused */
static const struct node guess = {
	50,
	&(struct node){
		25,
		&(struct node){
			12,
			(void *)0,
			(void *)0,
		},
		&(struct node){
			37,
			(void *)0,
			(void *)0,
		},
	},
	(void *)0
};
