/*	$NetBSD: gcc_attribute.c,v 1.11 2022/06/17 18:54:53 rillig Exp $	*/
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

/* expect+1: error: syntax error 'unknown_attribute' [249] */
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

/*
 * Ensure that __attribute__ can be specified everywhere in a declaration.
 * This is the simplest possible requirement that covers all valid code.
 * It accepts invalid code as well, but these cases are covered by GCC and
 * Clang already.
 *
 * Since lint only parses the attributes but doesn't really relate them to
 * identifiers or other entities, ensuring that valid code can be parsed is
 * enough for now.
 *
 * To really associate __attribute__ with the corresponding entity, the
 * grammar needs to be rewritten, see the example with __noreturn__ above.
 */
__attribute__((deprecated("d1")))
const
__attribute__((deprecated("d2")))
int
__attribute__((deprecated("d3")))
*
// The below line would produce a syntax error.
// __attribute__((deprecated("d3")))
const
__attribute__((deprecated("d4")))
identifier
__attribute__((deprecated("d5")))
(
    __attribute__((deprecated("d6")))
    void
    __attribute__((deprecated("d7")))
    )
    __attribute__((deprecated("d8")))
;

/*
 * The attribute 'const' provides stronger guarantees than 'pure', and
 * 'volatile' is not defined.  To keep the grammar simple, any T_QUAL is
 * allowed at this point, but only syntactically.
 */
int const_function(int) __attribute__((const));
/* cover 'gcc_attribute_spec: T_QUAL' */
/* expect+1: error: syntax error 'volatile' [249] */
int volatile_function(int) __attribute__((volatile));
