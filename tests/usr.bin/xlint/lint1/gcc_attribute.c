/*	$NetBSD: gcc_attribute.c,v 1.5 2021/05/03 07:08:54 rillig Exp $	*/
# 3 "gcc_attribute.c"

/*
 * Tests for the various attributes for functions, types, statements that are
 * provided by GCC.
 *
 * https://gcc.gnu.org/onlinedocs/gcc/Attribute-Syntax.html
 * https://gcc.gnu.org/onlinedocs/gcc/Function-Attributes.html
 * https://gcc.gnu.org/onlinedocs/gcc/Variable-Attributes.html
 * https://gcc.gnu.org/onlinedocs/gcc/Type-Attributes.html
 * https://gcc.gnu.org/onlinedocs/gcc/Enumerator-Attributes.html
 * https://gcc.gnu.org/onlinedocs/gcc/Statement-Attributes.html
 * https://gcc.gnu.org/onlinedocs/gcc/Label-Attributes.html
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
