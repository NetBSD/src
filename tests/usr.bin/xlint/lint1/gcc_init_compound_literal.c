/*	$NetBSD: gcc_init_compound_literal.c,v 1.1 2021/04/17 20:36:17 rillig Exp $	*/
# 3 "gcc_init_compound_literal.c"

/*
 * C99 says in 6.7.8p4:
 *
 *	All the expressions in an initializer for an object that has static
 *	storage duration shall be constant expressions or string literals.
 *
 * The term "constant expression" is defined in C99 6.6 and is quite
 * restricted, except for a single paragraph, 6.6p10:
 *
 *	An implementation may accept other forms of constant expressions.
 *
 * GCC additionally allows compound expressions, and these can even use the
 * array-to-pointer conversion from C99 6.3.2.1, which allows to initialize a
 * pointer object with a pointer to a direct-value statically allocated array.
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
