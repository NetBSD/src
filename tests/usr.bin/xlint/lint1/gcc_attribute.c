/*	$NetBSD: gcc_attribute.c,v 1.8 2021/07/06 18:43:27 rillig Exp $	*/
# 3 "gcc_attribute.c"

/*
 * Tests for the various attributes for functions, types, statements that are
 * provided by GCC.
 *
 * https://gcc.gnu.org/onlinedocs/gcc/Attribute-Syntax.html
 */

void __attribute__((noinline))
do_not_inline(void)
{
}

/* All pointer arguments must be nonnull. */
void __attribute__((nonnull))
function_nonnull(void *, const void *, int);

/*
 * The documentation suggests that the argument list of nonnull be nonempty,
 * but GCC 9.3.0 accepts an empty list as well, treating all parameters as
 * nonnull.
 */
void __attribute__((nonnull()))
function_nonnull_list(void *, const void *, int);

/* Arguments 1 and 2 must be nonnull. */
void __attribute__((nonnull(1, 2)))
function_nonnull_list(void *, const void *, int);

/* expect+1: syntax error 'unknown_attribute' */
void __attribute__((unknown_attribute))
function_with_unknown_attribute(void);

/*
 * There is an attribute called 'pcs', but that attribute must not prevent an
 * ordinary variable from being named the same.  Starting with scan.l 1.77
 * from 2017-01-07, that variable name generated a syntax error.  Fixed in
 * lex.c 1.33 from 2021-05-03.
 *
 * Seen in yds.c, function yds_allocate_slots.
 */
int
local_variable_pcs(void)
{
	int pcs = 3;
	return pcs;
}

/*
 * FIXME: The attributes are handled by different grammar rules even though
 *  they occur in the same syntactical position.
 *
 * Grammar rule abstract_decl_param_list handles the first attribute.
 *
 * Grammar rule direct_abstract_declarator handles all remaining attributes.
 *
 * Since abstract_decl_param_list contains type_attribute_opt, this could be
 * the source of the many shift/reduce conflicts in the grammar.
 */
int
func(
    int(int)
    __attribute__((__noreturn__))
    __attribute__((__noreturn__))
);

/*
 * https://gcc.gnu.org/onlinedocs/gcc/Attribute-Syntax.html says that the
 * attribute-list is a "possibly empty comma-separated sequence of
 * attributes".
 *
 * No matter whether this particular example is interpreted as an empty list
 * or a list containing a single empty attribute, the result is the same in
 * both cases.
 */
void one_empty_attribute(void)
    __attribute__((/* none */));

/*
 * https://gcc.gnu.org/onlinedocs/gcc/Attribute-Syntax.html further says that
 * each individual attribute may be "Empty. Empty attributes are ignored".
 */
void two_empty_attributes(void)
    __attribute__((/* none */, /* still none */));
