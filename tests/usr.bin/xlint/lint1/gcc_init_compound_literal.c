/*	$NetBSD: gcc_init_compound_literal.c,v 1.2 2021/04/17 20:57:18 rillig Exp $	*/
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
 */

// Seen in sys/crypto/aes/aes_ccm.c.
const struct {
    const unsigned char *ctxt;
} T = {
	(void *)0,
	(void *)0,	/* expect: too many struct/union initializers */
// FIXME: lint: assertion "sym->s_scl == EXTERN || sym->s_scl == STATIC" failed
//	.ctxt = (const unsigned char[4]){
//	    1, 2, 3, 4
//	},
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
/* TODO: activate after fixing the assertion failure
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
*/
